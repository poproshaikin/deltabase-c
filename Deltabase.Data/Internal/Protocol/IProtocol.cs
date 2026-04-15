using Deltabase.Data.Internal.Models;

namespace Deltabase.Data.Internal.Protocol;

internal interface IProtocol
{
    byte[] Encode(DeltabaseMessage message); 
    DeltabaseMessage Parse(byte[] data);
}