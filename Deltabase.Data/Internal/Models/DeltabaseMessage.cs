namespace Deltabase.Data.Internal.Models;

internal enum NetErrorCode : byte
{
    Success = 0,
    ProtocolViolation = 1,
    DbNotExist = 2,
}

internal abstract class DeltabaseMessage(MessageType type)
{
    public MessageType Type { get; set; } = type;
}

internal class PingMessage() : DeltabaseMessage(MessageType.Ping);

internal class PongMessage(Guid sessionId, NetErrorCode errorCode = NetErrorCode.Success)
    : DeltabaseMessage(MessageType.Pong)
{
    public Guid SessionId  { get; set; } = sessionId;
    public NetErrorCode ErrorCode { get; set; } = errorCode;
    public IReadOnlyList<byte> Payload { get; set; } = [];
}

internal class QueryMessage(Guid sessionId, string query) : DeltabaseMessage(MessageType.Query)
{
    public Guid SessionId { get; set; } = sessionId;
    public string Query { get; set; } = query;
}

internal class CreateDbMessage(Guid sessionId, string dbName) : DeltabaseMessage(MessageType.CreateDb)
{
    public Guid SessionId { get; set; } = sessionId;
    public string DbName { get; set; } = dbName;
}

internal class AttachDbMessage(Guid sessionId, string dbName) : DeltabaseMessage(MessageType.AttachDb)
{
    public Guid SessionId { get; set; } = sessionId;
    public string DbName { get; set; } = dbName;
}

internal class CloseMessage(Guid sessionId) : DeltabaseMessage(MessageType.Close)
{
    public Guid SessionId { get; set; } = sessionId;
}