namespace Deltabase.Data.Internal.Models;

internal enum MessageType 
{
    Undefined = 0,
    Ping = 1,
    Pong, 
    Query,
    CreateDb,
    AttachDb,
    CancelStream,
    Close
}