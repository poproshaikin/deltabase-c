namespace Deltabase.Data.Internal.Models;

internal enum NetErrorCode : byte
{
    Success = 0,
    StartStream,
    StreamChunk,
    EndStream,
    
    ProtocolViolation = 100,
    DbNotExist,
    SqlError,
    UninitializedSession
}

internal abstract class DeltabaseMessage(MessageType type, int requestId)
{
    public MessageType Type { get; set; } = type;

    public int RequestId { get; set; } = requestId;
}

internal class PingMessage(int requestId) : DeltabaseMessage(MessageType.Ping, requestId);

internal class PongMessage(Guid sessionId, int requestId, NetErrorCode errorCode, IReadOnlyList<byte> payload)
    : DeltabaseMessage(MessageType.Pong, requestId)
{
    public Guid SessionId  { get; set; } = sessionId;
    public NetErrorCode ErrorCode { get; set; } = errorCode;
    public IReadOnlyList<byte> Payload { get; set; } = payload;
}

internal class QueryMessage(Guid sessionId, int requestId, string query) : DeltabaseMessage(MessageType.Query, requestId)
{
    public Guid SessionId { get; set; } = sessionId;
    public string Query { get; set; } = query;
}

internal class CreateDbMessage(Guid sessionId, int requestId, string dbName) : DeltabaseMessage(MessageType.CreateDb, requestId)
{
    public Guid SessionId { get; set; } = sessionId;
    public string DbName { get; set; } = dbName;
}

internal class AttachDbMessage(Guid sessionId, int requestId, string dbName) : DeltabaseMessage(MessageType.AttachDb, requestId)
{
    public Guid SessionId { get; set; } = sessionId;
    public string DbName { get; set; } = dbName;
}

internal class CloseMessage(Guid sessionId, int requestId) : DeltabaseMessage(MessageType.Close, requestId)
{
    public Guid SessionId { get; set; } = sessionId;
}

internal class CancelStreamMessage(Guid sessionId, int requestId) : DeltabaseMessage(MessageType.CancelStream, requestId)
{
    public Guid SessionId { get; set; } = sessionId;
}