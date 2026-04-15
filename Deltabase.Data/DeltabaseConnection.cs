using System.Data;
using System.Data.Common;
using System.Diagnostics.CodeAnalysis;
using System.Net.Sockets;
using Deltabase.Data.Internal;
using Deltabase.Data.Internal.Utils;

namespace Deltabase.Data;

public class DeltabaseConnection : DbConnection
{
    private string _connectionString;
    private ConnectionParams _connectionParams;
    private ConnectionState _connectionState = ConnectionState.Closed;
    private DeltabaseConnector _connector;
    private Guid _sessionId;
    private bool _disposed;

    public DeltabaseConnection(string connectionString)
    {
        _connectionString = connectionString;
        _connectionParams = ParseConnectionString(connectionString);
        _connector = new DeltabaseConnector(_connectionParams.Host, _connectionParams.Port, SocketType.Stream, ProtocolType.Tcp);
        _disposed = false;
    }

    [AllowNull]
    public override string ConnectionString
    {
        get =>  _connectionString;
#pragma warning disable CS8601 // Possible null reference assignment.
        set =>  _connectionString = value;
#pragma warning restore CS8601 // Possible null reference assignment.
    }

    public override string Database => "deltabase";
    public override string DataSource => "deltabase";
    public override string ServerVersion => "1.0.0";
    public override ConnectionState State => _connectionState;

    public override void Open()
    {
        _connector.Connect();
        _sessionId = _connector.Open();
        _connectionState = ConnectionState.Open;
        _connector.AttachDatabase(_connectionParams.Database, _sessionId);
    }

    public override void ChangeDatabase(string databaseName)
    {
        throw new NotImplementedException();
    }

    public override void Close()
    {
        _connector.Close(_sessionId);
    }

    protected override DbCommand CreateDbCommand()
    {
        throw new NotImplementedException();
    }
    
    protected override DbTransaction BeginDbTransaction(IsolationLevel isolationLevel)
    {
        throw new NotImplementedException();
    }

    private static ConnectionParams ParseConnectionString(string connectionString)
    {
        var split = connectionString.Split(';');
        return new ConnectionParams(split[0], ushort.Parse(split[1]), split[2]); // host;port;database
    }

    protected override void Dispose(bool disposing)
    {
        if (_disposed) return;
        
        if (disposing)
        {
            Close();
            _connector?.Dispose();
        }
        
        _disposed = true;
        base.Dispose(disposing);
    }
}
