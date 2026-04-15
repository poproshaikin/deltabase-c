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
        get => _commandText;
        set => _commandText = value;
    }
    
    public override int CommandTimeout { get; set; }

    public override CommandType CommandType { get; set; }

    public override UpdateRowSource UpdatedRowSource { get; set; } 
    
    public override bool DesignTimeVisible { get; set; }

    protected override DbConnection? DbConnection
    {
        get => _connection;
        set => _connection = value as DeltabaseConnection ?? throw new DeltabaseException("DbConnection must be a DeltabaseConnection");
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
        Connection = connection;
    }

    public override int ExecuteNonQuery()
    {
        
    }

    public override object? ExecuteScalar()
    {
        throw new NotImplementedException();
    }

    public override void Prepare()
    {
        // Does not supported yet
    }

    protected override DbParameter CreateDbParameter()
    {
        throw new NotImplementedException();
    }

    protected override DbDataReader ExecuteDbDataReader(CommandBehavior behavior)
    {
        throw new NotImplementedException();
    }
    
    public override void Cancel()
    {
        throw new NotImplementedException();
    }

    private void Execute()
    {
        ArgumentNullException.ThrowIfNull(_connection);
        ArgumentNullException.ThrowIfNull(_commandText);

        var data = _connection.Connector.ExecuteCommand();
    }
}