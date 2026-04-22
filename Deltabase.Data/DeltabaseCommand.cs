using System.Data;
using System.Data.Common;
using System.Diagnostics.CodeAnalysis;
using Deltabase.Data.Internal.Utils;

namespace Deltabase.Data;

public class DeltabaseCommand : DbCommand
{
    private string? _commandText;
    private DeltabaseConnection? _connection;

    [AllowNull]
    public override string CommandText
    {
        get => _commandText ?? string.Empty;
        set => _commandText = value;
    }
    
    public override int CommandTimeout { get; set; }

    public override CommandType CommandType { get; set; }

    public override UpdateRowSource UpdatedRowSource { get; set; } 
    
    public override bool DesignTimeVisible { get; set; }

    protected override DbConnection? DbConnection
    {
        get => _connection;
        set => _connection = 
            value as DeltabaseConnection ??
            throw new DeltabaseException("DbConnection must be a DeltabaseConnection");
    }

    protected override DbParameterCollection DbParameterCollection { get; }
    
    protected override DbTransaction? DbTransaction { get; set; }

    public DeltabaseCommand()
    {
        DbParameterCollection = new DeltabaseParameterCollection();
    }

    public DeltabaseCommand(string? commandText) : this()
    {
        _commandText = commandText;
    }

    public DeltabaseCommand(string? commandText, DeltabaseConnection connection) : this(commandText)
    {
        _connection = connection;
    }

    public override int ExecuteNonQuery()
    {
        ArgumentNullException.ThrowIfNull(_connection);
        ArgumentNullException.ThrowIfNull(_commandText);

        var table = _connection.Connector.ExecuteCommand(_commandText, _connection.SessionId);

        if (table.Rows.Length > 0 && table.Rows[0].Values.Length > 0)
            return (int)(table.Rows[0].Values[0] ?? 0);

        return 0;
    }

    public override object? ExecuteScalar()
    {
        ArgumentNullException.ThrowIfNull(_connection);
        ArgumentNullException.ThrowIfNull(_commandText);
        
        var table = _connection.Connector.ExecuteCommand(_commandText, _connection.SessionId);
        
        if (table.Rows.Length > 0 && table.Rows[0].Values.Length > 0)
            return table.Rows[0].Values[0];

        return null;
    }

    public override void Prepare()
    {
        // Does not support yet
    }

    protected override DbParameter CreateDbParameter()
    {
        throw new NotImplementedException();
    }

    protected override DeltabaseDataReader ExecuteDbDataReader(CommandBehavior behavior)
    {
        ArgumentNullException.ThrowIfNull(_connection);
        ArgumentNullException.ThrowIfNull(_commandText);

        var table = _connection.Connector.ExecuteCommand(_commandText, _connection.SessionId);
        
        return new DeltabaseDataReader(table);
    }
    
    public override void Cancel()
    {
        throw new NotImplementedException();
    }
}