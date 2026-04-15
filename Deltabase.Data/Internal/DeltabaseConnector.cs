using System.Net.NetworkInformation;
using System.Net.Sockets;
using Deltabase.Data.Internal.Models;
using Deltabase.Data.Internal.Protocol;
using Deltabase.Data.Internal.Utils;

namespace Deltabase.Data.Internal;

internal class DeltabaseConnector
{
    private string _host;
    private ushort _port;

    private SocketWrapper _socketWrapper;
    
    public DeltabaseConnector(string host, ushort port, SocketType socketType, ProtocolType protocol, IProtocol? binaryProtocol = null) 
    {
        _host = host;
        _port = port;
        _socketWrapper = new SocketWrapper(
            new Socket(AddressFamily.InterNetwork, SocketType.Stream, ProtocolType.Tcp), 
            binaryProtocol ?? new StdProtocolImpl()
        );
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
}