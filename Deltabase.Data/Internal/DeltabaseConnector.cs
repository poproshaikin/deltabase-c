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

    private SocketWrapper _socketWrapper;
    private IProtocol _protocol;
    
    public DeltabaseConnector(string host, ushort port, SocketType socketType, ProtocolType networkProtocol, IProtocol? binaryProtocol = null) 
    {
        _host = host;
        _port = port;
        _protocol = binaryProtocol ?? new StdProtocolImpl();
        _socketWrapper = new SocketWrapper(AddressFamily.InterNetwork, socketType, networkProtocol, _protocol);
    }

    public void Connect()
    {
        _socketWrapper.Socket.Connect(_host, _port);
    }

    public Guid Open()
    {
        var ping = new PingMessage();
        _socketWrapper.Send(ping);
        
        var response = _socketWrapper.Receive();
        
        if (response is not PongMessage pong)
            throw new DeltabaseException();
        if (pong.ErrorCode != NetErrorCode.Success)
            throw new DeltabaseException(pong.ErrorCode);

        return pong.SessionId;
    }

    public void AttachDatabase(string database, Guid sessionId)
    {
        var attachDb = new AttachDbMessage(sessionId, database);
        _socketWrapper.Send(attachDb);
        var response = _socketWrapper.Receive();
        
        if (response is not PongMessage pong)
            throw new DeltabaseException();
        if (pong.ErrorCode != NetErrorCode.Success)
            throw new DeltabaseException(pong.ErrorCode);
    }

    public void Close(Guid sessionId)
    {
        var close = new CloseMessage(sessionId);
        _socketWrapper.Send(close);
        _socketWrapper.Close();
    }

    public void Dispose()
    {
        _socketWrapper.Dispose();
    }

    public Table ExecuteCommand(string command, Guid sessionId)
    {
        var message = new QueryMessage(sessionId, command);
        _socketWrapper.Send(message);
        var response = _socketWrapper.Receive();
        
        if (response is not PongMessage pong)
            throw new DeltabaseException();
        if (pong.ErrorCode != NetErrorCode.Success)
            throw new DeltabaseException(pong.ErrorCode);

        return _protocol.ParseTable(pong.Payload);
    }
}