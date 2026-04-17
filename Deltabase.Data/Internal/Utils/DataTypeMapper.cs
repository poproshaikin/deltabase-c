using System.Text;
using Deltabase.Data.Internal.Models;

namespace Deltabase.Data.Internal.Utils;

internal static class DataTypeMapper
{
    public static Type? ToType(DataType dataType)
    {
        return dataType switch
        {
            DataType.Undefined => throw new DeltabaseException(),
            DataType.Null => null,
            DataType.Integer => typeof(int),
            DataType.Real => typeof(double),
            DataType.Char => typeof(char),
            DataType.Bool => typeof(bool),
            DataType.String => typeof(string),
            _ => throw new ArgumentOutOfRangeException(nameof(dataType), dataType, null)
        };
    }
    
    public static DataType FromType(Type? type) =>
        type switch
        {
            null => DataType.Null,
            _ when type == typeof(int) => DataType.Integer,
            _ when type == typeof(double)
                   || type == typeof(float)
                   || type == typeof(decimal)
                => DataType.Real,
            _ when type == typeof(char) => DataType.Char,
            _ when type == typeof(bool) => DataType.Bool,
            _ when type == typeof(string) => DataType.String,
            _ => DataType.Undefined
        };

    public static ulong GetDataTypeSize(DataType type)
    {
        return type switch
        {
            DataType.Undefined => throw new ArgumentOutOfRangeException(nameof(type), type, null),
            DataType.Null => 0,
            DataType.Integer => 4,
            DataType.Real => 8,
            DataType.Char => 1,
            DataType.Bool => 1,
            DataType.String => 0,
            _ => throw new ArgumentOutOfRangeException(nameof(type), type, null)
        };
    }

    public static bool IsFixedSize(DataType type)
    {
        return type switch
        {
            DataType.Undefined => throw new ArgumentOutOfRangeException(nameof(type), type, null),
            DataType.Null => true,
            DataType.Integer => true,
            DataType.Real => true,
            DataType.Char => true,
            DataType.Bool => true,
            DataType.String => false,
            _ => throw new ArgumentOutOfRangeException(nameof(type), type, null)
        };
    }

    public static object? ToObject(byte[] bytes, DataType type)
    {
        if (IsFixedSize(type))
            if (GetDataTypeSize(type) != (ulong)bytes.Length)
                throw new ArgumentOutOfRangeException(nameof(type), type, null);
        
        return type switch
        {
            DataType.Undefined => throw new ArgumentOutOfRangeException(nameof(type), type, null),
            DataType.Null => null,
            DataType.Integer => BitConverter.ToInt32(bytes, 0),
            DataType.Real => BitConverter.ToDouble(bytes, 0),
            DataType.Bool => BitConverter.ToBoolean(bytes, 0),
            DataType.Char => (char)bytes[0],
            DataType.String => Encoding.UTF8.GetString(bytes),
            _ => throw new ArgumentOutOfRangeException(nameof(type), type, null)
        };
    }
}