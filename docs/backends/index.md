# Supported Backends and Features

Follow the links to learn more about each backend and detailed supported features.

<table>
  <tbody>
    <tr>
      <th></th>
      <th><a href="oracle">Oracle</a></th>
      <th><a href="postgresql">PostgreSQL</a></th>
      <th><a href="mysql">MySQL</a></th>
      <th><a href="sqlite3">SQLite3</a></th>
      <th><a href="firebird">Firebird</a></th>
      <th><a href="odbc">ODBC</a></th>
      <th><a href="db2">DB2</a></th>
    </tr>
    <tr>
      <td>Binding by Name</td>
      <td>YES</td>
      <td>YES (>=8.0)</td>
      <td>YES</td>
      <td>YES</td>
      <td>YES</td>
      <td>YES</td>
      <td>YES</td>
    </tr>
    <tr>
      <td>Dynamic Binding</td>
      <td>YES</td>
      <td>YES</td>
      <td>YES</td>
      <td>YES</td>
      <td>YES</td>
      <td>YES</td>
      <td></td>
    </tr>
    <tr>
      <td>Bulk Operations</td>
      <td>YES</td>
      <td>YES</td>
      <td>YES</td>
      <td>YES</td>
      <td>YES</td>
      <td>YES</td>
      <td>YES</td>
    </tr>
    <tr>
      <td>Transactions</td>
      <td>YES</td>
      <td>YES</td>
      <td>YES (>=4.0)</td>
      <td>YES</td>
      <td>YES</td>
      <td>YES</td>
      <td>YES</td>
    </tr>
    <tr>
      <td>BLOB Data Type</td>
      <td>YES</td>
      <td>YES</td>
      <td>YES (mapped to `std::string`)</td>
      <td>YES</td>
      <td>YES</td>
      <td>NO</td>
      <td>NO</td>
    </tr>
    <tr>
      <td>RowID Data Type</td>
      <td>YES</td>
      <td>YES</td>
      <td>NO</td>
      <td>NO</td>
      <td>NO</td>
      <td>NO</td>
      <td>NO</td>
    </tr>
    <tr>
      <td>Nested Statements</td>
      <td>YES</td>
      <td>NO</td>
      <td>NO</td>
      <td>NO</td>
      <td>NO</td>
      <td>NO</td>
      <td>YES</td>
    </tr>
    <tr>
      <td>Stored Procedures</td>
      <td>YES</td>
      <td>YES</td>
      <td>NO (but stored functions, YES)</td>
      <td>NO</td>
      <td>YES</td>
      <td>NO</td>
      <td>YES</td>
    </tr>
  </tbody>
</table>
