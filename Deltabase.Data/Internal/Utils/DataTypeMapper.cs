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
}