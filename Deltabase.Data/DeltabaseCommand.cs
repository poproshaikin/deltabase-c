using System.Data;
using System.Data.Common;
using System.Diagnostics.CodeAnalysis;
using Deltabase.Data.Internal.Models;
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
        return ExecuteNonQueryAsync(CancellationToken.None).GetAwaiter().GetResult();
    }

    public override object? ExecuteScalar()
    {
        return ExecuteScalarAsync(CancellationToken.None).GetAwaiter().GetResult();
    }

    public override void Prepare()
    {
        // Does not support yet
    }

    protected override DbParameter CreateDbParameter()
    {
        throw new NotImplementedException();
    }

    protected override DbDataReader ExecuteDbDataReader(CommandBehavior behavior)
    {
        return ExecuteDbDataReaderAsync(behavior, CancellationToken.None).GetAwaiter().GetResult();
    }

    protected override async Task<DbDataReader> ExecuteDbDataReaderAsync(CommandBehavior behavior, CancellationToken cancellationToken)
    {
        ArgumentNullException.ThrowIfNull(_connection);
        ArgumentNullException.ThrowIfNull(_commandText);

        var dataEnumerator = await _connection.Connector.ExecuteCommandAsync(_commandText, 
            _connection.SessionId, 
            cancellationToken);
        
        return new DeltabaseDataReader(dataEnumerator);
    }

    public override Task<int> ExecuteNonQueryAsync(CancellationToken cancellationToken)
    {
        return ExecuteNonQueryAsyncCore(cancellationToken);
    }

    public override Task<object?> ExecuteScalarAsync(CancellationToken cancellationToken)
    {
        return ExecuteScalarAsyncCore(cancellationToken);
    }

    public override void Cancel()
    {
        throw new NotImplementedException();
    }

    private async Task<int> ExecuteNonQueryAsyncCore(CancellationToken cancellationToken)
    {
        var table = await ReadFirstTableAsync(cancellationToken);

        if (table is { Rows.Length: > 0 } && table.Value.Rows[0].Values.Length > 0)
        {
            return Convert.ToInt32(table.Value.Rows[0].Values[0] ?? 0);
        }

        return 0;
    }

    private async Task<object?> ExecuteScalarAsyncCore(CancellationToken cancellationToken)
    {
        var table = await ReadFirstTableAsync(cancellationToken);

        if (table is { Rows.Length: > 0 } && table.Value.Rows[0].Values.Length > 0)
        {
            return table.Value.Rows[0].Values[0];
        }

        return null;
    }

    private async Task<Table?> ReadFirstTableAsync(CancellationToken cancellationToken)
    {
        ArgumentNullException.ThrowIfNull(_connection);
        ArgumentNullException.ThrowIfNull(_commandText);

        var data = await _connection.Connector.ExecuteCommandAsync(
            _commandText,
            _connection.SessionId,
            cancellationToken);

        await foreach (var table in data.WithCancellation(cancellationToken))
        {
            return table;
        }

        return null;
    }
}