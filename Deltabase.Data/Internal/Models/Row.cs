namespace Deltabase.Data.Internal.Models;

internal record struct Row(object?[] Values)
{
    public object? this[int index] => Values?[index];
}