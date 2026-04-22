using System.Data;
using System.Net.NetworkInformation;
using System.Net.Sockets;
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
        var ping = new PingMessage();
        _socketHandle.Send(ping);
        
        var response = _socketHandle.Receive();
        ThrowIfUnsuccessful(response, out var pong);

        return pong.SessionId;
    }

    public void AttachDatabase(string database, Guid sessionId)
    {
        var attachDb = new AttachDbMessage(sessionId, database);
        _socketHandle.Send(attachDb);
        var response = _socketHandle.Receive();
        
        ThrowIfUnsuccessful(response);
    }

    public void Close(Guid sessionId)
    {
        var close = new CloseMessage(sessionId);
        _socketHandle.Send(close);
        _socketHandle.Close();
    }

    public void Dispose()
    {
        _socketHandle.Dispose();
    }

    public Task<IAsyncEnumerable<DataTable>> ExecuteCommand(string command, Guid sessionId)
    {
        var message = new QueryMessage(sessionId, command);
        _socketHandle.Send(message);
        var response = _socketHandle.Receive();
        ThrowIfUnsuccessful(response, out var pong);
        
        return ReceiveStreamAsync(message.SessionId);
    }

    private async Task<IAsyncEnumerable<DataTable>> ReceiveStreamAsync(Guid sessionId)
    {
        var startStream = await _socketHandle.ReceiveAsync();
        ThrowIfUnsuccessful(startStream, out var pong);
        if (pong.ErrorCode != NetErrorCode.StartStream ||
            pong.SessionId != sessionId)
            throw new DeltabaseException(NetErrorCode.ProtocolViolation);
        
        1
    }

    private void ThrowIfUnsuccessful(DeltabaseMessage response)
    {
        if (response is not PongMessage pong)
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
}