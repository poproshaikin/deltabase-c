using Deltabase.Data.Internal.Models;

namespace Deltabase.Data.Internal.Utils;

public class DeltabaseException : Exception
{
    public DeltabaseException(string? message = null) : base(message)
    {
    }
    
    internal DeltabaseException(NetErrorCode err) : base(err.ToString())
    {
    }
}