using System.Collections;
using System.Data.Common;
using Deltabase.Data.Internal.Models;

namespace Deltabase.Data;

public class DeltabaseDataReader : DbDataReader
{
    private Table _table;
    private int _cursor;
    
    internal DeltabaseDataReader(Table table)
    {
        _table = table;
        _cursor = 0;
    }

    public override bool HasRows => _table.Rows.Length > 0;

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

    public override string GetDataTypeName(int ordinal)
    {
        return _table.Columns[ordinal].GetType().ToString();
    }

    public override DateTime GetDateTime(int ordinal)
    {
        throw new NotImplementedException();
    }

    public override decimal GetDecimal(int ordinal)
    {
        throw new NotImplementedException();
    }

    public override double GetDouble(int ordinal)
    {
        return GetFieldAtCursor<double>(ordinal);
    }

    public override Type GetFieldType(int ordinal)
    {
        return _table.Columns[ordinal].GetType();
    }

    public override float GetFloat(int ordinal)
    {
        throw new NotImplementedException();
    }

    public override Guid GetGuid(int ordinal)
    {
        throw new NotImplementedException();
    }

    public override short GetInt16(int ordinal)
    {
        throw new NotImplementedException();
    }

    public override int GetInt32(int ordinal)
    {
        return GetFieldAtCursor<int>(ordinal);
    }

    public override long GetInt64(int ordinal)
    {
        throw new NotImplementedException();
    }

    public override string GetName(int ordinal)
    {
        return _table.Columns[ordinal].Name;
    }

    public override int GetOrdinal(string name)
    {
        return _table.Columns.IndexOf(_table.Columns.First(c => c.Name == name));
    }

    public override string GetString(int ordinal)
    {
        return GetFieldAtCursor<string>(ordinal) ?? throw new InvalidCastException();
    }

    public override object GetValue(int ordinal)
    {
        return GetFieldAtCursor<object>(ordinal);
    }

    public override int GetValues(object[] values)
    {
        throw new NotImplementedException();
    }

    public override bool IsDBNull(int ordinal)
    {
        return _table.Rows[_cursor][ordinal] == null; ;
    }

    public override int FieldCount => _table.Columns.Length;

    public override object this[int ordinal] => GetValue(ordinal);

    public override object this[string name] => GetValue(GetOrdinal(name));

    public override int RecordsAffected { get; }

    public override bool IsClosed { get; }

    public override bool NextResult()
    {
        throw new NotImplementedException();
    }

    public override bool Read()
    {
        if (_table.Rows == null! || _table.Rows.Length == 0) 
            return false;
        
        _cursor++;
        
        if (_cursor >= _table.Rows.Length) 
            return false;

        return true;
    }

    public override int Depth { get; }

    public override IEnumerator GetEnumerator()
    {
        throw new NotImplementedException();
    }
    
    private T GetFieldAtCursor<T>(int ordinal)
    {
        if (_table.Rows[_cursor][ordinal] == null) throw new InvalidCastException();
        
        return (T)_table.Rows[_cursor][ordinal]!;
    }
}