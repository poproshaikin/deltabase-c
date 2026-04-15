using System.Net.Sockets;
using System.Buffers.Binary;
using Deltabase.Data.Internal.Models;
using Deltabase.Data.Internal.Protocol;

namespace Deltabase.Data.Internal.Utils;

internal class SocketWrapper : IDisposable
{
    public Socket Socket { get; set; }
    public IProtocol Protocol { get; set; }

    public SocketWrapper(Socket socket, IProtocol protocol)
    {
        Socket = socket;
        Protocol = protocol;
    }

    public void Send(DeltabaseMessage message)
    {
        var payload = Protocol.Encode(message);
        var header = new byte[sizeof(uint)];

        BinaryPrimitives.WriteUInt32BigEndian(header, (uint)payload.Length);

        SendAll(header);

        if (payload.Length > 0)
        {
            SendAll(payload);
        }
    }

    public DeltabaseMessage Receive()
    {
        var header = ReceiveExact(sizeof(uint));
        var payloadLength = BinaryPrimitives.ReadUInt32BigEndian(header);

        if (payloadLength > int.MaxValue)
        {
            throw new InvalidOperationException("Incoming payload is too large.");
        }

        var payload = payloadLength == 0
            ? []
            : ReceiveExact((int)payloadLength);

        return Protocol.Parse(payload);
    }

    private void SendAll(byte[] bytes)
    {
        var totalSent = 0;

        while (totalSent < bytes.Length)
        {
            var sent = Socket.Send(bytes, totalSent, bytes.Length - totalSent, SocketFlags.None);

            if (sent <= 0)
            {
                throw new SocketException((int)SocketError.ConnectionReset);
            }

            totalSent += sent;
        }
    }

    private byte[] ReceiveExact(int length)
    {
        var buffer = new byte[length];
        var totalRead = 0;

        while (totalRead < length)
        {
            var read = Socket.Receive(buffer, totalRead, length - totalRead, SocketFlags.None);

            if (read <= 0)
            {
                throw new SocketException((int)SocketError.ConnectionReset);
            }

            totalRead += read;
        }

        return buffer;
    }

    public void Close()
    {
        Socket.Close();
    }

    public void Dispose()
    {
        Socket.Dispose();
    }
}