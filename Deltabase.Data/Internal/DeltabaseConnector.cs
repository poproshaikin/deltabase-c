using System.Data;
using System.Net.Sockets;
using System.Runtime.CompilerServices;
using System.Threading;
using Deltabase.Data.Internal.Models;
using Deltabase.Data.Internal.Protocol;
using Deltabase.Data.Internal.Utils;

namespace Deltabase.Data.Internal;

internal class DeltabaseConnector : IDisposable
{
    private string _host;
    private ushort _port;

    private SocketHandle _socketHandle;
    private IProtocol _protocol;
    private CancellationTokenSource _cancellationTokenSource;
    private int _lastRequestId;
    
    public DeltabaseConnector(string host, ushort port, SocketType socketType, ProtocolType networkProtocol, IProtocol? binaryProtocol = null) 
    {
        _host = host;
        _port = port;
        _protocol = binaryProtocol ?? new StdProtocolImpl();
        _socketHandle = new SocketHandle(AddressFamily.InterNetwork, socketType, networkProtocol, _protocol);
        _cancellationTokenSource = new CancellationTokenSource();
        _lastRequestId = 1;
    }

    public void Connect()
    {
        _socketHandle.Socket.Connect(_host, _port);
    }

    public Guid Open()
    {
        var requestId = NextRequestId();
        var ping = new PingMessage(requestId);
        _socketHandle.Send(ping);
        
        var response = _socketHandle.Receive();
        ThrowIfUnsuccessful(response, requestId, out var pong);

        return pong.SessionId;
    }

    public void AttachDatabase(string database, Guid sessionId)
    {
        var requestId = NextRequestId();
        var attachDb = new AttachDbMessage(sessionId, requestId, database);
        _socketHandle.Send(attachDb);
        var response = _socketHandle.Receive();
        
        ThrowIfUnsuccessful(response, requestId);
    }

    public void Close(Guid sessionId)
    {
        var requestId = NextRequestId();
        var close = new CloseMessage(sessionId, requestId);
        _socketHandle.Send(close);
        _socketHandle.Close();
    }

    public void Dispose()
    {
        _socketHandle.Dispose();
    }

    public async Task<IAsyncEnumerable<Table>> ExecuteCommandAsync(string command, Guid sessionId, CancellationToken cancellationToken)
    {
        var requestId = NextRequestId();
        var message = new QueryMessage(sessionId, requestId, command);
        _socketHandle.Send(message);
        var response = await _socketHandle.ReceiveAsync();
        ThrowIfUnsuccessful(response, requestId, out var pong);
        
        return ReceiveStreamAsync(message.SessionId, requestId, cancellationToken);
    }

    private async IAsyncEnumerable<Table> ReceiveStreamAsync(Guid sessionId, 
        int requestId, 
        [EnumeratorCancellation] CancellationToken cancellationToken)
    {
        var startStream = await _socketHandle.ReceiveAsync();
        ThrowIfUnsuccessful(startStream, requestId, out var startStreamPong);
        if (startStreamPong.ErrorCode != NetErrorCode.StartStream ||
            startStreamPong.SessionId != sessionId)
            throw new DeltabaseException(NetErrorCode.ProtocolViolation);
        
        while (!cancellationToken.IsCancellationRequested)
        {
            var message = await _socketHandle.ReceiveAsync();
            ThrowIfUnsuccessful(message, requestId, out var chunkPong);

            if (chunkPong.ErrorCode == NetErrorCode.EndStream)
                break;
            
            if (chunkPong.ErrorCode != NetErrorCode.StreamChunk ||
                chunkPong.SessionId != sessionId ||
                chunkPong.RequestId != requestId)
                throw new DeltabaseException(NetErrorCode.ProtocolViolation);

            yield return _protocol.ParseTable(chunkPong.Payload);
        }
    }

    private void ThrowIfUnsuccessful(DeltabaseMessage response)
    {
        if (response is not PongMessage pong)
            throw new DeltabaseException(NetErrorCode.ProtocolViolation);
        if (pong.ErrorCode >= (NetErrorCode)100) // Is a failure code
            throw new DeltabaseException(pong.ErrorCode);
    }

    private void ThrowIfUnsuccessful(DeltabaseMessage response, int requestId)
    {
        if (response is not PongMessage pong)
            throw new DeltabaseException(NetErrorCode.ProtocolViolation);

        if (pong.RequestId != requestId)
            throw new DeltabaseException(NetErrorCode.ProtocolViolation);

        if (pong.ErrorCode >= (NetErrorCode)100) // Is a failure code
            throw new DeltabaseException(pong.ErrorCode);
    }

    private void ThrowIfUnsuccessful(DeltabaseMessage response, out PongMessage @out)
    {
        if (response is not PongMessage pong)
            throw new DeltabaseException(NetErrorCode.ProtocolViolation);
        if (pong.ErrorCode >= (NetErrorCode)100) // Is a failure code
            throw new DeltabaseException(pong.ErrorCode);
        
        @out = pong;
    }

    private void ThrowIfUnsuccessful(DeltabaseMessage response, int requestId, out PongMessage @out)
    {
        if (response is not PongMessage pong)
            throw new DeltabaseException(NetErrorCode.ProtocolViolation);

        if (pong.RequestId != requestId)
            throw new DeltabaseException(NetErrorCode.ProtocolViolation);

        if (pong.ErrorCode >= (NetErrorCode)100) // Is a failure code
            throw new DeltabaseException(pong.ErrorCode);

        @out = pong;
    }

    private int NextRequestId()
    {
        return Interlocked.Increment(ref _lastRequestId);
    }
}