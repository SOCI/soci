# Data Types

## Static binding

The static binding for types is most useful when the types used in the database are known at compile time - this was already presented above with the help of `into` and `use` functions.

The following types are currently supported for use with `into` and `use` expressions:

* `char` (for character values)
* `int8_t`, `uint8_t`, `int16_t`, `uint16_t`, `int32_t`, `uint32_t`, `int64_t`, `uint64_t`, `double` (for numeric values)
* `std::string` (for string values)
* `std::tm` (for datetime values)
* `soci::statement` (for nested statements and PL/SQL cursors)
* `soci::blob` (for Binary Large OBjects)
* `soci::row_id` (for row identifiers)

See the test code that accompanies the library to see how each of these types is used.

Note that the fixed-size types `int32_t`, `int64_t` etc. are only typedefs for the fundamental types `int`, `long` etc.
This means that the two can be used interchangeably when interacting with SOCI and you can choose which one of them you want to use in your code.

### Static binding for bulk operations

Bulk inserts, updates, and selects are supported through the following `std::vector` based into and use types:

* `std::vector<char>`
* `std::vector<int8_t>`
* `std::vector<uint8_t>`
* `std::vector<int16_t>`
* `std::vector<uint16_t>`
* `std::vector<int32_t>`
* `std::vector<uint32_t>`
* `std::vector<int64_t>`
* `std::vector<uint64_t>`
* `std::vector<double>`
* `std::vector<std::string>`
* `std::vector<std::tm>`

Use of the vector based types mirrors that of the standard types, with the size of the vector used to specify the number of records to process at a time.
See below for examples.

Bulk operations are supported also for `std::vector`s of the user-provided types that have appropriate conversion routines defines.

## Dynamic binding

For certain applications it is desirable to be able to select data from arbitrarily structured tables (e.g. via "`select * from ...`") and format the resulting data based upon its type.

SOCI supports binding dynamic resultset through the `soci::row` and `soci::column_properties` classes.

Data is selected into a `row` object, which holds `column_properties` objects describing the attributes of data contained in each column.
Once the data type for each column is known, the data can be formatted appropriately.

For example, the code below creates an XML document from a selected row of data from an arbitrary table:

```cpp
row r;
sql << "select * from some_table", into(r);

std::ostringstream doc;
doc << "<row>" << std::endl;
for(std::size_t i = 0; i != r.size(); ++i)
{
    const column_properties & props = r.get_properties(i);

    doc << '<' << props.get_name() << '>';

    switch(props.get_db_type())
    {
    case db_string:
        doc << r.get<std::string>(i);
        break;
    case db_double:
        doc << r.get<double>(i);
        break;
    case db_int8:
        doc << r.get<int8_t>(i);
        break;
    case db_uint8:
        doc << r.get<uint8_t>(i);
        break;
    case db_int16:
        doc << r.get<int16_t>(i);
        break;
    case db_uint16:
        doc << r.get<uint16_t>(i);
        break;
    case db_int32:
        doc << r.get<int32_t>(i);
        break;
    case db_uint32:
        doc << r.get<uint32_t>(i);
        break;
    case db_int64:
        doc << r.get<int64_t>(i);
        break;
    case db_uint64:
        doc << r.get<uint64_t>(i);
        break;
    case db_date:
        std::tm when = r.get<std::tm>(i);
        doc << asctime(&when);
        break;
    }

    doc << "</" << props.get_name() << '>' << std::endl;
}
doc << "</row>";
```

The type `T` parameter that should be passed to `row::get<T>()` depends on the SOCI data type that is returned from `data_type column_properties::get_data_type()` or `db_type column_properties::get_db_type()`.
Users are encouraged to use the latter as it supports a wider range of numerical C++ types.

`row::get<T>()` throws an exception of type `std::bad_cast` if an incorrect type `T` is requested.

| SOCI Data Type (`data_type`) | `row::get<T>` specialization |
|------------------------------|------------------------------|
| `dt_double`                  | `double`                     |
| `dt_integer`                 | `int`                        |
| `dt_long_long`               | `long long`                  |
| `dt_unsigned_long_long`      | `unsigned long long`         |
| `dt_string`                  | `std::string`                |
| `dt_date`                    | `std::tm`                    |

| SOCI Data Type (`db_type`)   | `row::get<T>` specialization |
|------------------------------|------------------------------|
| `db_double`                  | `double`                     |
| `db_int8`                    | `int8_t`                     |
| `db_uint8`                   | `uint8_t`                    |
| `db_int16`                   | `int16_t`                    |
| `db_uint16`                  | `uint16_t`                   |
| `db_int32`                   | `int32_t`                    |
| `db_uint32`                  | `uint32_t`                   |
| `db_int64`                   | `int64_t`                    |
| `db_uint64`                  | `uint64_t`                   |
| `db_string`                  | `std::string`                |
| `db_date`                    | `std::tm`                    |

The mapping of underlying database column types to SOCI datatypes is database specific.
See the [backend documentation](backends/index.md) for details.

The `row` also provides access to indicators for each column:

```cpp
row r;
sql << "select name from some_table where id = 1", into(r);
if (r.get_indicator(0) != soci::i_null)
{
    std::cout << r.get<std::string>(0);
}
```

It is also possible to extract data from the `row` object using its stream-like interface, where each extracted variable should have matching type respective to its position in the chain:

```cpp
row r;
sql << "select name, address, age from persons where id = 123", into(r);

string name, address;
int age;

r >> name >> address >> age;
```

Note, however, that this interface is *not* compatible with the standard `std::istream` class and that it is only possible to extract a single row at a time - for "safety" reasons the row boundary is preserved and it is necessary to perform the `fetch` operation explicitly for each consecutive row.

## User-defined C++ types

SOCI can be easily extended with support for user-defined datatypes.

The extension mechanism relies on appropriate specialization of the `type_conversion` structure that converts to and from one of the following SOCI base types:

* `double`
* `int8_t`
* `uint8_t`
* `int16_t`
* `uint16_t`
* `int32_t`
* `uint32_t`
* `int64_t`
* `uint64_t`
* `std::string`
* `char`
* `std::tm`

There are three required class members for a valid `type_conversion` specialization:

* the `base_type` type definition, aliasing either one of the base types *or another user-defined type*
* the `from_base()` static member function, converting from the base type
* the `to_base()` static member function, converting to the base type

Note that no database-specific code is required to define user conversion.

The following example shows how the user can extend SOCI to support his own type `MyInt`, which here is some wrapper for the fundamental `int` type:

```cpp
class MyInt
{
public:
    MyInt() {}
    MyInt(int i) : i_(i) {}

    void set(int i) { i_ = i; }
    int get() const { return i_; }

private:
    int i_;
};

namespace soci
{
    template <>
    struct type_conversion<MyInt>
    {
        typedef int base_type;

        static void from_base(int i, indicator ind, MyInt & mi)
        {
            if (ind == i_null)
            {
                throw soci_error("Null value not allowed for this type");
            }

            mi.set(i);
        }

        static void to_base(const MyInt & mi, int & i, indicator & ind)
        {
            i = mi.get();
            ind = i_ok;
        }
    };
}
```

The above specialization for `soci::type_conversion<MyInt>` is enough to enable the following:

```cpp
MyInt i;

sql << "select count(*) from person", into(i);

cout << "We have " << i.get() << " persons in the database.\n";
```

Note that there is a number of types from the Boost library integrated with SOCI out of the box, see [Integration with Boost](boost.md) for complete description. Use these as examples of conversions for more complext data types.

Another possibility to extend SOCI with custom data types is to use the `into_type<T>` and `use_type<T>` class templates, which specializations can be user-provided. These specializations need to implement the interface defined by, respectively, the `into_type_base` and `use_type_base`
classes.

Note that when specializing these template classes the only convention is that when the indicator
variable is used (see below), it should appear in the second position. Please refer to the library source code to see how this is done for the standard types.

## Object-Relational Mapping

SOCI provides a class called `values` specifically to enable object-relational mapping via `type_conversion` specializations.

For example, the following code maps a `Person` object to and from a database table containing columns `"ID"`, `"FIRST_NAME"`, `"LAST_NAME"`, and `"GENDER"`.

Note that the mapping is non-invasive - the `Person` object itself does not contain any SOCI-specific code:

```cpp
struct Person
{
    int id;
    std::string firstName;
    std::string lastName;
    std::string gender;
};

namespace soci
{
    template<>
    struct type_conversion<Person>
    {
        typedef values base_type;

        static void from_base(values const & v, indicator /* ind */, Person & p)
        {
            p.id = v.get<int>("ID");
            p.firstName = v.get<std::string>("FIRST_NAME");
            p.lastName = v.get<std::string>("LAST_NAME");

            // p.gender will be set to the default value "unknown"
            // when the column is null:
            p.gender = v.get<std::string>("GENDER", "unknown");

            // alternatively, the indicator can be tested directly:
            // if (v.indicator("GENDER") == i_null)
            // {
            //     p.gender = "unknown";
            // }
            // else
            // {
            //     p.gender = v.get<std::string>("GENDER");
            // }
        }

        static void to_base(const Person & p, values & v, indicator & ind)
        {
            v.set("ID", p.id);
            v.set("FIRST_NAME", p.firstName);
            v.set("LAST_NAME", p.lastName);
            v.set("GENDER", p.gender, p.gender.empty() ? i_null : i_ok);
            ind = i_ok;
        }
    };
}
```

With the above `type_conversion` specialization in place, it is possible to use `Person` directly with SOCI:

```cpp
session sql(oracle, "service=db1 user=scott password=tiger");

Person p;
p.id = 1;
p.lastName = "Smith";
p.firstName = "Pat";
sql << "insert into person(id, first_name, last_name) "
        "values(:ID, :FIRST_NAME, :LAST_NAME)", use(p);

Person p1;
sql << "select * from person", into(p1);
assert(p1.id == 1);
assert(p1.firstName + p.lastName == "PatSmith");
assert(p1.gender == "unknown");

p.firstName = "Patricia";
sql << "update person set first_name = :FIRST_NAME "
        "where id = :ID", use(p);
```

Note: The `values` class is currently not suited for use outside of `type_conversion`specializations.
It is specially designed to facilitate object-relational mapping when used as shown above.
