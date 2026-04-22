namespace Deltabase.Data.Internal.Models;

internal enum DataType : ulong
{
    Undefined = 0,
    Null = 1,
    Integer = 2,
    Real,
    Char, 
    Bool,
    String
}