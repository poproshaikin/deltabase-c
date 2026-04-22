using System.Collections;
using System.Data.Common;
using Deltabase.Data.Internal.Models;

namespace Deltabase.Data;

public class DeltabaseDataReader : DbDataReader
{
    private readonly IAsyncEnumerator<Table> _tables;
    private Table? _currentTable;
    private int _rowIndex = -1;
    private bool _isClosed;

    internal DeltabaseDataReader(IAsyncEnumerable<Table> dataEnumerator)
    {
        _tables = dataEnumerator.GetAsyncEnumerator();
    }

    public override bool HasRows => _currentTable is { } table && table.Rows.Length > 0;

    public override bool IsClosed => _isClosed;

    public override int FieldCount => _currentTable is { } table ? table.Columns.Length : 0;

    public override int Depth => 0;

    public override int RecordsAffected => -1;

    public override object this[int ordinal] => GetValue(ordinal);

    public override object this[string name] => GetValue(GetOrdinal(name));

    public override bool Read()
    {
        return ReadAsync(CancellationToken.None).GetAwaiter().GetResult();
    }

    public override async Task<bool> ReadAsync(CancellationToken cancellationToken)
    {
        ThrowIfClosed();

        while (true)
        {
            if (_currentTable is { } table && _rowIndex + 1 < table.Rows.Length)
            {
                _rowIndex++;
                return true;
            }

            if (!await MoveNextTableAsync(cancellationToken).ConfigureAwait(false))
            {
                return false;
            }
        }
    }

    public override bool NextResult()
    {
        return false;
    }

    public override Task<bool> NextResultAsync(CancellationToken cancellationToken)
    {
        return Task.FromResult(false);
    }

    public override void Close()
    {
        if (_isClosed)
        {
            return;
        }

        _tables.DisposeAsync().AsTask().GetAwaiter().GetResult();
        _currentTable = null;
        _rowIndex = -1;
        _isClosed = true;
    }

    public override string GetName(int ordinal)
    {
        return GetCurrentTable().Columns[ordinal].Name;
    }

    public override string GetDataTypeName(int ordinal)
    {
        return GetFieldType(ordinal).Name;
    }

    public override Type GetFieldType(int ordinal)
    {
        return GetCurrentTable().Columns[ordinal].Type ?? typeof(object);
    }

    public override int GetOrdinal(string name)
    {
        var columns = GetCurrentTable().Columns;
        for (var i = 0; i < columns.Length; i++)
        {
            if (string.Equals(columns[i].Name, name, StringComparison.Ordinal))
            {
                return i;
            }
        }

        throw new IndexOutOfRangeException($"Column '{name}' was not found.");
    }

    public override object GetValue(int ordinal)
    {
        return GetCurrentRow()[ordinal] ?? DBNull.Value;
    }

    public override int GetValues(object[] values)
    {
        var currentTable = GetCurrentTable();
        var currentRow = GetCurrentRow();
        var count = Math.Min(values.Length, currentTable.Columns.Length);

        for (var i = 0; i < count; i++)
        {
            values[i] = currentRow[i] ?? DBNull.Value;
        }

        return count;
    }

    public override bool IsDBNull(int ordinal)
    {
        return GetCurrentRow()[ordinal] is null;
    }

    public override bool GetBoolean(int ordinal)
    {
        return GetFieldAtCursor<bool>(ordinal);
    }

    public override byte GetByte(int ordinal)
    {
        return GetFieldAtCursor<byte>(ordinal);
    }

    public override long GetBytes(int ordinal, long dataOffset, byte[]? buffer, int bufferOffset, int length)
    {
        throw new NotImplementedException();
    }

    public override char GetChar(int ordinal)
    {
        return GetFieldAtCursor<char>(ordinal);
    }

    public override long GetChars(int ordinal, long dataOffset, char[]? buffer, int bufferOffset, int length)
    {
        throw new NotImplementedException();
    }

    public override DateTime GetDateTime(int ordinal)
    {
        return GetFieldAtCursor<DateTime>(ordinal);
    }

    public override decimal GetDecimal(int ordinal)
    {
        return GetFieldAtCursor<decimal>(ordinal);
    }

    public override double GetDouble(int ordinal)
    {
        return GetFieldAtCursor<double>(ordinal);
    }

    public override float GetFloat(int ordinal)
    {
        return GetFieldAtCursor<float>(ordinal);
    }

    public override Guid GetGuid(int ordinal)
    {
        return GetFieldAtCursor<Guid>(ordinal);
    }

    public override short GetInt16(int ordinal)
    {
        return GetFieldAtCursor<short>(ordinal);
    }

    public override int GetInt32(int ordinal)
    {
        return GetFieldAtCursor<int>(ordinal);
    }

    public override long GetInt64(int ordinal)
    {
        return GetFieldAtCursor<long>(ordinal);
    }

    public override string GetString(int ordinal)
    {
        return GetFieldAtCursor<string>(ordinal) ?? throw new InvalidCastException();
    }

    public override IEnumerator GetEnumerator()
    {
        throw new NotImplementedException();
    }

    private async Task<bool> MoveNextTableAsync(CancellationToken cancellationToken)
    {
        if (!await _tables.MoveNextAsync().ConfigureAwait(false))
        {
            _currentTable = null;
            _rowIndex = -1;
            return false;
        }

        cancellationToken.ThrowIfCancellationRequested();

        _currentTable = _tables.Current;
        _rowIndex = -1;
        return true;
    }

    private Table GetCurrentTable()
    {
        return _currentTable ?? throw new InvalidOperationException("No current table is loaded.");
    }

    private Row GetCurrentRow()
    {
        var table = GetCurrentTable();
        if (_rowIndex < 0 || _rowIndex >= table.Rows.Length)
        {
            throw new InvalidOperationException("No current row is loaded.");
        }

        return table.Rows[_rowIndex];
    }

    private T GetFieldAtCursor<T>(int ordinal)
    {
        var value = GetCurrentRow()[ordinal];
        if (value is null)
        {
            throw new InvalidCastException();
        }

        return (T)value;
    }

    private void ThrowIfClosed()
    {
        if (_isClosed)
        {
            throw new InvalidOperationException("The reader is closed.");
        }
    }
}
