using Deltabase.Data.Internal.Models;
using System.Buffers.Binary;
using System.IO;
using System.Text;

namespace Deltabase.Data.Internal.Protocol;

internal class StdProtocolImpl : IProtocol
{
    public byte[] Encode(DeltabaseMessage message)
    {
        using var stream = new MemoryStream();
        using var writer = new BinaryWriter(stream, Encoding.UTF8, leaveOpen: true);

        WriteMessageType(writer, message.Type);

        switch (message)
        {
            case PingMessage:
                return stream.ToArray();

            case PongMessage pong:
                WriteGuid(writer, pong.SessionId);
                writer.Write((byte)pong.ErrorCode);
                return stream.ToArray();

            case QueryMessage query:
                WriteGuid(writer, query.SessionId);
                WriteString(writer, query.Query);
                return stream.ToArray();

            case CreateDbMessage createDb:
                WriteGuid(writer, createDb.SessionId);
                WriteString(writer, createDb.DbName);
                return stream.ToArray();

            case AttachDbMessage attachDb:
                WriteGuid(writer, attachDb.SessionId);
                WriteString(writer, attachDb.DbName);
                return stream.ToArray();

            case CloseMessage close:
                WriteGuid(writer, close.SessionId);
                return stream.ToArray();

            default:
                return ProtocolViolationMessageBytes();
        }
    }

    public DeltabaseMessage Parse(byte[] data)
    {
        using var stream = new MemoryStream(data, writable: false);
        using var reader = new BinaryReader(stream, Encoding.UTF8, leaveOpen: true);

        if (!TryReadByte(reader, out var rawType))
        {
            return ProtocolViolationMessage();
        }

        var messageType = (MessageType)rawType;

        switch (messageType)
        {
            case MessageType.Ping:
                return new PingMessage();

            case MessageType.Pong:
                if (!TryReadGuid(reader, out var pongSessionId))
                {
                    return ProtocolViolationMessage();
                }

                if (!TryReadByte(reader, out var rawErr))
                {
                    return ProtocolViolationMessage();
                }

                return new PongMessage(pongSessionId, (NetErrorCode)rawErr);

            case MessageType.Query:
                if (!TryReadGuid(reader, out var querySessionId))
                {
                    return ProtocolViolationMessage();
                }

                if (!TryReadString(reader, out var query))
                {
                    return ProtocolViolationMessage();
                }

                return new QueryMessage(querySessionId, query);

            case MessageType.CreateDb:
                if (!TryReadGuid(reader, out var createSessionId))
                {
                    return ProtocolViolationMessage();
                }

                if (!TryReadString(reader, out var createDbName))
                {
                    return ProtocolViolationMessage();
                }

                return new CreateDbMessage(createSessionId, createDbName)
                {
                    SessionId = createSessionId,
                };

            case MessageType.AttachDb:
                if (!TryReadGuid(reader, out var attachSessionId))
                {
                    return ProtocolViolationMessage();
                }

                if (!TryReadString(reader, out var attachDbName))
                {
                    return ProtocolViolationMessage();
                }

                return new AttachDbMessage(attachSessionId, attachDbName)
                {
                    SessionId = attachSessionId,
                };

            case MessageType.Close:
                if (!TryReadGuid(reader, out var closeSessionId))
                {
                    return ProtocolViolationMessage();
                }

                return new CloseMessage(closeSessionId);

            case MessageType.Undefined:
            default:
                return ProtocolViolationMessage();
        }
    }

    private static void WriteMessageType(BinaryWriter writer, MessageType messageType)
    {
        writer.Write((byte)messageType);
    }

    private static void WriteGuid(BinaryWriter writer, Guid value)
    {
        Span<byte> uuidBytes = stackalloc byte[16];
        if (!value.TryWriteBytes(uuidBytes, bigEndian: true, out var bytesWritten) || bytesWritten != 16)
        {
            throw new InvalidOperationException("Failed to serialize Guid to UUID bytes.");
        }

        writer.Write(uuidBytes);
    }

    private static void WriteString(BinaryWriter writer, string value)
    {
        var payload = Encoding.UTF8.GetBytes(value);
        WriteUInt64BigEndian(writer, (ulong)payload.Length);

        if (payload.Length > 0)
        {
            writer.Write(payload);
        }
    }

    private static bool TryReadByte(BinaryReader reader, out byte value)
    {
        if (reader.BaseStream.Position + sizeof(byte) > reader.BaseStream.Length)
        {
            value = default;
            return false;
        }

        value = reader.ReadByte();
        return true;
    }

    private static bool TryReadGuid(BinaryReader reader, out Guid value)
    {
        const int guidSize = 16;

        if (reader.BaseStream.Position + guidSize > reader.BaseStream.Length)
        {
            value = Guid.Empty;
            return false;
        }

        var uuidBytes = reader.ReadBytes(guidSize);

        if (uuidBytes.Length != guidSize)
        {
            value = Guid.Empty;
            return false;
        }

        value = new Guid(uuidBytes, bigEndian: true);
        return true;
    }

    private static bool TryReadString(BinaryReader reader, out string value)
    {
        value = string.Empty;

        if (reader.BaseStream.Position + sizeof(ulong) > reader.BaseStream.Length)
        {
            return false;
        }

        if (!TryReadUInt64BigEndian(reader, out var length))
        {
            return false;
        }

        if (length > int.MaxValue)
        {
            return false;
        }

        var payloadLength = (int)length;

        if (reader.BaseStream.Position + payloadLength > reader.BaseStream.Length)
        {
            return false;
        }

        var payload = reader.ReadBytes(payloadLength);
        value = payloadLength == 0 ? string.Empty : Encoding.UTF8.GetString(payload);
        return true;
    }

    private static void WriteUInt64BigEndian(BinaryWriter writer, ulong value)
    {
        Span<byte> bytes = stackalloc byte[sizeof(ulong)];
        BinaryPrimitives.WriteUInt64BigEndian(bytes, value);
        writer.Write(bytes);
    }

    private static bool TryReadUInt64BigEndian(BinaryReader reader, out ulong value)
    {
        value = default;

        if (reader.BaseStream.Position + sizeof(ulong) > reader.BaseStream.Length)
        {
            return false;
        }

        Span<byte> bytes = stackalloc byte[sizeof(ulong)];
        var read = reader.Read(bytes);

        if (read != sizeof(ulong))
        {
            return false;
        }

        value = BinaryPrimitives.ReadUInt64BigEndian(bytes);
        return true;
    }

    private static DeltabaseMessage ProtocolViolationMessage()
    {
        return new CloseMessage(Guid.Empty);
    }

    private byte[] ProtocolViolationMessageBytes()
    {
        return Encode(ProtocolViolationMessage());
    }
}