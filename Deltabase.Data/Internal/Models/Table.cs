namespace Deltabase.Data.Internal.Models;

internal record struct Table(Row Columns, IReadOnlyList<Row> Rows);