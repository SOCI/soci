//
// Copyright (C) 2004-2024 Maciej Sobczak, Stephen Hutton
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// https://www.boost.org/LICENSE_1_0.txt)
//

#include "soci/soci.h"

#include <catch.hpp>

#include "test-context.h"

#include <vector>
#include <list>
#include <set>

namespace soci
{

namespace tests
{

// This variable is referenced from test-common.cpp to force linking this file.
volatile bool soci_use_test_lob = false;

// Helper function used in some tests below. Generates an XML sample about
// approximateSize bytes long.
static std::string make_long_xml_string(int approximateSize = 5000)
{
    const int tagsSize = 6 + 7;
    const int patternSize = 26;
    const int patternsCount = approximateSize / patternSize + 1;

    std::string s;
    s.reserve(tagsSize + patternsCount * patternSize);

    std::ostringstream ss;
    ss << "<test size=\"" << approximateSize << "\">";
    for (int i = 0; i != patternsCount; ++i)
    {
       ss << "ABCDEFGHIJKLMNOPQRSTUVWXYZ";
    }
    ss << "</test>";

    return ss.str();
}

// The helper function to remove trailing \n from a given string.
// Used for XML strings, returned from the DB.
// The returned XML value doesn't need to be identical to the original one as
// string, only structurally equal as XML. In particular, extra whitespace
// can be added and this does happen with Oracle, for example, which adds
// an extra new line, so remove it if it's present.
static std::string remove_trailing_nl(std::string str)
{
    if (!str.empty() && *str.rbegin() == '\n')
    {
        str.resize(str.length() - 1);
    }

    return str;
}

TEST_CASE_METHOD(common_tests, "CLOB", "[core][clob]")
{
    soci::session sql(backEndFactory_, connectString_);

    auto_table_creator tableCreator(tc_.table_creator_clob(sql));
    if (!tableCreator.get())
    {
        WARN("CLOB type not supported by the database, skipping the test.");
        return;
    }

    long_string s1; // empty
    sql << "insert into soci_test(id, s) values (1, :s)", use(s1);

    long_string s2;
    s2.value = "hello";
    sql << "select s from soci_test where id = 1", into(s2);

    CHECK(s2.value.size() == 0);

    s1.value = make_long_xml_string();

    sql << "update soci_test set s = :s where id = 1", use(s1);

    sql << "select s from soci_test where id = 1", into(s2);

    CHECK(s2.value == s1.value);

    // Check that trailing new lines are preserved.
    s1.value = "multi\nline\nstring\n\n";
    sql << "update soci_test set s = :s where id = 1", use(s1);
    sql << "select s from soci_test where id = 1", into(s2);
    CHECK(tc_.fix_crlf_if_necessary(s2.value) == s1.value);
}

TEST_CASE_METHOD(common_tests, "CLOB vector", "[core][clob][vector]")
{
    soci::session sql(backEndFactory_, connectString_);

    auto_table_creator tableCreator(tc_.table_creator_clob(sql));
    if (!tableCreator.get())
    {
        WARN("CLOB type not supported by the database, skipping the test.");
        return;
    }

    std::vector<int> ids(2);
    ids[0] = 1;
    ids[1] = 2;
    std::vector<long_string> s1(2); // empty values
    sql << "insert into soci_test(id, s) values (:id, :s)", use(ids), use(s1);

    std::vector<long_string> s2(2);
    s2[0].value = "hello_1";
    s2[1].value = "hello_2";
    sql << "select s from soci_test", into(s2);

    REQUIRE(s2.size() == 2);
    CHECK(s2[0].value.empty());
    CHECK(s2[1].value.empty());

    s1[0].value = make_long_xml_string();
    s1[1].value = make_long_xml_string(10000);

    sql << "update soci_test set s = :s where id = :id", use(s1), use(ids);

    sql << "select s from soci_test", into(s2);

    REQUIRE(s2.size() == 2);
    CHECK(s2[0].value == s1[0].value);
    CHECK(s2[1].value == s1[1].value);
}

TEST_CASE_METHOD(common_tests, "XML", "[core][xml]")
{
    soci::session sql(backEndFactory_, connectString_);

    auto_table_creator tableCreator(tc_.table_creator_xml(sql));
    if (!tableCreator.get())
    {
        WARN("XML type not supported by the database, skipping the test.");
        return;
    }

    int id = 1;
    xml_type xml;
    xml.value = make_long_xml_string();

    sql << "insert into soci_test (id, x) values (:1, "
        << tc_.to_xml(":2")
        << ")",
        use(id), use(xml);

    xml_type xml2;

    sql << "select "
        << tc_.from_xml("x")
        << " from soci_test where id = :1",
        into(xml2), use(id);

    CHECK(xml.value == remove_trailing_nl(xml2.value));

    sql << "update soci_test set x = null where id = :1", use(id);

    indicator ind;
    sql << "select "
        << tc_.from_xml("x")
        << " from soci_test where id = :1",
        into(xml2, ind), use(id);

    CHECK(ind == i_null);

    // Inserting malformed XML into an XML column must fail but some backends
    // (e.g. Firebird) don't have real XML support, so exclude them from this
    // test.
    if (tc_.has_real_xml_support())
    {
        xml.value = "<foo></not_foo>";
        CHECK_THROWS_AS(
            (sql << "insert into soci_test(id, x) values (2, "
                        + tc_.to_xml(":1") + ")",
                    use(xml)
            ), soci_error
        );
    }
}

// Tha same test as above, but using vectors of xml_type values.
TEST_CASE_METHOD(common_tests, "XML vector", "[core][xml][vector]")
{
    soci::session sql(backEndFactory_, connectString_);

    auto_table_creator tableCreator(tc_.table_creator_xml(sql));
    if (!tableCreator.get())
    {
        WARN("XML type not supported by the database, skipping the test.");
        return;
    }

    std::vector<int> id(2);
    id[0] = 1;
    id[1] = 1; // Use the same ID to select both objects by ID.
    std::vector<xml_type> xml(2);
    xml[0].value = make_long_xml_string();
    // Check long strings handling.
    xml[1].value = make_long_xml_string(10000);

    sql << "insert into soci_test (id, x) values (:1, "
        << tc_.to_xml(":2")
        << ")",
        use(id), use(xml);

    std::vector<xml_type> xml2(2);

    sql << "select "
        << tc_.from_xml("x")
        << " from soci_test where id = :1",
        into(xml2), use(id.at(0));

    CHECK(xml.at(0).value == remove_trailing_nl(xml2.at(0).value));
    CHECK(xml.at(1).value == remove_trailing_nl(xml2.at(1).value));

    sql << "update soci_test set x = null where id = :1", use(id.at(0));

    std::vector<indicator> ind(2);
    sql << "select "
        << tc_.from_xml("x")
        << " from soci_test where id = :1",
        into(xml2, ind), use(id.at(0));

    CHECK(ind.at(0) == i_null);
    CHECK(ind.at(1) == i_null);
}

TEST_CASE_METHOD(common_tests, "XML and int vectors", "[core][xml][vector]")
{
    soci::session sql(backEndFactory_, connectString_);

    auto_table_creator tableCreator(tc_.table_creator_xml(sql));
    if (!tableCreator.get())
    {
        WARN("XML type not supported by the database, skipping the test.");
        return;
    }

    std::vector<int> id(3);
    id[0] = 0;
    id[1] = 1;
    id[2] = 2;
    std::vector<xml_type> xml(3);
    std::vector<indicator> ind(3);
    xml[0].value = make_long_xml_string();
    ind[0] = i_ok;
    ind[1] = i_null;
    // Check long strings handling.
    xml[2].value = make_long_xml_string(10000);
    ind[2] = i_ok;

    sql << "insert into soci_test (id, x) values (:1, "
        << tc_.to_xml(":2")
        << ")",
        use(id), use(xml, ind);

    std::vector<int> id2(3);
    std::vector<xml_type> xml2(3);
    std::vector<indicator> ind2(3);

    sql << "select id, "
        << tc_.from_xml("x")
        << " from soci_test order by id",
        into(id2), into(xml2, ind2);

    CHECK(id.at(0) == id2.at(0));
    CHECK(id.at(1) == id2.at(1));
    CHECK(id.at(2) == id2.at(2));

    CHECK(xml.at(0).value == remove_trailing_nl(xml2.at(0).value));
    CHECK(xml.at(2).value == remove_trailing_nl(xml2.at(2).value));

    CHECK(ind.at(0) == ind2.at(0));
    CHECK(ind.at(1) == ind2.at(1));
    CHECK(ind.at(2) == ind2.at(2));
}

TEST_CASE_METHOD(common_tests, "Into XML vector with several fetches", "[core][xml][into][vector][statement]")
{
    int stringSize = 0;
    SECTION("short string")
    {
        stringSize = 100;
    }
    SECTION("long string")
    {
        stringSize = 10000;
    }

    // Skip the rest when not executing the current section.
    if (!stringSize)
        return;

    soci::session sql(backEndFactory_, connectString_);

    auto_table_creator tableCreator(tc_.table_creator_xml(sql));
    if (!tableCreator.get())
    {
        WARN("XML type not supported by the database, skipping the test.");
        return;
    }

    int const count = 5;
    std::vector<xml_type> values(count);
    for (int i = 0; i != count; ++i)
        values[i].value = make_long_xml_string(stringSize + i*100);

    sql << "insert into soci_test (x) values ("
        << tc_.to_xml(":2")
        << ")",
        use(values);

    std::vector<xml_type> result(3);
    soci::statement st = (sql.prepare <<
        "select " << tc_.from_xml("x") << " from soci_test", into(result));

    st.execute(true);
    REQUIRE(result.size() == 3);
    CHECK(remove_trailing_nl(result[0].value) == values[0].value);
    CHECK(remove_trailing_nl(result[1].value) == values[1].value);
    CHECK(remove_trailing_nl(result[2].value) == values[2].value);

    REQUIRE(st.fetch());
    REQUIRE(result.size() == 2);
    CHECK(remove_trailing_nl(result[0].value) == values[3].value);
    CHECK(remove_trailing_nl(result[1].value) == values[4].value);

    REQUIRE(!st.fetch());
}

TEST_CASE_METHOD(common_tests, "BLOB", "[core][blob]")
{
    soci::session sql(backEndFactory_, connectString_);

    auto_table_creator tableCreator(tc_.table_creator_blob(sql));

    if (!tableCreator.get())
    {
        try
        {
            soci::blob blob(sql);
            FAIL("BLOB creation should throw, if backend doesn't support BLOBs");
        }
        catch (const soci_error &)
        {
            // Throwing is expected if the backend doesn't support BLOBs
        }
        WARN("BLOB type not supported by the database, skipping the test.");
        return;
    }

    const char dummy_data[] = "abcdefghijklmnopqrstuvwxyz";

    // Cross-DB usage of BLOBs is only possible if the entire lifetime of the blob object
    // is covered in an active transaction.
    soci::transaction transaction(sql);
    SECTION("Read-access on just-constructed blob")
    {
        soci::blob blob(sql);

        CHECK(blob.get_len() == 0);

        char buf[5];
        std::size_t read_bytes = blob.read_from_start(buf, sizeof(buf));

        // There should be no data that could be read
        CHECK(read_bytes == 0);

        // Reading from any offset other than zero is invalid
        CHECK_THROWS_AS(blob.read_from_start(buf, sizeof(buf), 1), soci_error);
    }
    SECTION("validity & initialization")
    {
        soci::blob blob;
        CHECK_FALSE(blob.is_valid());
        blob.initialize(sql);
        CHECK(blob.is_valid());

        soci::blob other;
        CHECK_FALSE(other.is_valid());
        other = std::move(blob);
        CHECK(other.is_valid());
        CHECK_FALSE(blob.is_valid());
    }
    SECTION("BLOB I/O")
    {
        soci::blob blob(sql);

        std::size_t written_bytes = blob.write_from_start(dummy_data, 5);

        CHECK(written_bytes == 5);
        CHECK(blob.get_len() == 5);

        char buf[5];
        static_assert(sizeof(buf) <= sizeof(dummy_data), "Underlying assumption violated");

        std::size_t read_bytes = blob.read_from_start(buf, sizeof(buf));

        CHECK(read_bytes == sizeof(buf));

        for (std::size_t i = 0; i < sizeof(buf); ++i)
        {
            CHECK(buf[i] == dummy_data[i]);
        }

        written_bytes = blob.append(dummy_data + 5, 3);

        CHECK(written_bytes == 3);
        CHECK(blob.get_len() == 8);

        read_bytes = blob.read_from_start(buf, sizeof(buf), 3);

        CHECK(read_bytes == 5);

        for (std::size_t i = 0; i < sizeof(buf); ++i)
        {
            CHECK(buf[i] == dummy_data[i + 3]);
        }

        blob.trim(2);

        CHECK(blob.get_len() == 2);

        read_bytes = blob.read_from_start(buf, sizeof(buf));

        CHECK(read_bytes == 2);

        for (std::size_t i = 0; i < read_bytes; ++i)
        {
            CHECK(buf[i] == dummy_data[i]);
        }

        // Reading from an offset >= the current length of the blob is invalid
        CHECK_THROWS_AS(blob.read_from_start(buf, sizeof(buf), blob.get_len()), soci_error);

        written_bytes = blob.append("z", 1);

        CHECK(written_bytes == 1);
        CHECK(blob.get_len() == 3);

        read_bytes = blob.read_from_start(buf, 1, 2);

        CHECK(read_bytes == 1);
        CHECK(buf[0] == 'z');

        // Writing more than one position beyond the blob is invalid
        // (Writing exactly one position beyond is the same as appending)
        CHECK_THROWS_AS(blob.write_from_start(dummy_data, 2, blob.get_len() + 1), soci_error);
    }
    SECTION("Inserting/Reading default-constructed blob")
    {
        {
            soci::blob input_blob(sql);

            sql << "insert into soci_test (id, b) values(5, :b)", soci::use(input_blob);
        }

        soci::blob output_blob(sql);
        soci::indicator ind;

        sql << "select b from soci_test where id = 5", soci::into(output_blob, ind);

        CHECK(ind == soci::i_ok);
        CHECK(output_blob.get_len() == 0);
    }
    SECTION("Ensure reading into blob overwrites previous contents")
    {
        soci::blob blob(sql);
        blob.write_from_start("hello kitty", 10);

        CHECK(blob.get_len() == 10);

        {
            soci::blob write_blob(sql);
            write_blob.write_from_start("test", 4);
            sql << "insert into soci_test (id, b) values (5, :b)", soci::use(write_blob);
        }

        sql << "select b from soci_test where id = 5", soci::into(blob);

        CHECK(blob.get_len() == 4);
        char buf[5];

        std::size_t read_bytes = blob.read_from_start(buf, sizeof(buf));
        CHECK(read_bytes == 4);

        CHECK(buf[0] == 't');
        CHECK(buf[1] == 'e');
        CHECK(buf[2] == 's');
        CHECK(buf[3] == 't');
    }
    SECTION("Blob-DB interaction")
    {
        soci::blob write_blob(sql);

        static_assert(sizeof(dummy_data) >= 10, "Underlying assumption violated");
        write_blob.write_from_start(dummy_data, 10);

        const int first_id = 42;

        // Write and retrieve blob from/into database
        sql << "insert into soci_test (id, b) values(:id, :b)", soci::use(first_id), soci::use(write_blob);

        // Append to write_blob - these changes must not reflect in the BLOB stored in the DB
        write_blob.append("ay", 2);
        CHECK(write_blob.get_len() == 12);
        char buf[15];
        std::size_t read_bytes = write_blob.read_from_start(buf, sizeof(buf));
        CHECK(read_bytes == 12);
        for (std::size_t i = 0; i < 10; ++i)
        {
            CHECK(buf[i] == dummy_data[i]);
        }
        CHECK(buf[10] == 'a');
        CHECK(buf[11] == 'y');


        soci::blob read_blob(sql);
        sql << "select b from soci_test where id = :id", soci::use(first_id), soci::into(read_blob);
        CHECK(sql.got_data());

        CHECK(read_blob.get_len() == 10);

        std::size_t bytes_read = read_blob.read_from_start(buf, sizeof(buf));
        CHECK(bytes_read == read_blob.get_len());
        CHECK(bytes_read == 10);
        for (std::size_t i = 0; i < bytes_read; ++i)
        {
            CHECK(buf[i] == dummy_data[i]);
        }

        // Update original blob and insert new db-entry (must not change previous entry)
        const int second_id = first_id + 1;
        write_blob.trim(0);
        static_assert(sizeof(dummy_data) >= 15 + 5, "Underlying assumption violated");
        write_blob.write_from_start(dummy_data + 15, 5);

        sql << "insert into soci_test (id, b) values (:id, :b)", soci::use(second_id), soci::use(write_blob);

        // First, check that the original entry has not been changed
        sql << "select b from soci_test where id = :id", soci::use(first_id), soci::into(read_blob);
        CHECK(read_blob.get_len() == 10);

        // Then check new entry can be read
        sql << "select b from soci_test where id = :id", soci::use(second_id), soci::into(read_blob);

        bytes_read = read_blob.read_from_start(buf, sizeof(buf));
        CHECK(bytes_read == read_blob.get_len());
        CHECK(bytes_read == 5);
        for (std::size_t i = 0; i < bytes_read; ++i)
        {
            CHECK(buf[i] == dummy_data[i + 15]);
        }
    }
    SECTION("Binary data")
    {
        const std::uint8_t binary_data[12] = {0, 1, 2, 3, 4, 5, 6, 7, 22, 255, 250 };

        soci::blob write_blob(sql);

        std::size_t bytes_written = write_blob.write_from_start(binary_data, sizeof(binary_data));
        CHECK(bytes_written == sizeof(binary_data));

        sql << "insert into soci_test (id, b) values (1, :b)", soci::use(write_blob);

        soci::blob read_blob(sql);

        sql << "select b from soci_test where id = 1", soci::into(read_blob);

        CHECK(read_blob.get_len() == sizeof(binary_data));

        std::uint8_t buf[20];
        std::size_t bytes_read = read_blob.read_from_start(buf, sizeof(buf));

        CHECK(bytes_read == sizeof(binary_data));
        for (std::size_t i = 0; i < sizeof(binary_data); ++i)
        {
            CHECK(buf[i] == binary_data[i]);
        }
    }
    SECTION("Rowset Blob recognition")
    {
        soci::blob blob(sql);

        // Write and retrieve blob from/into database
        int id = 1;
        sql << "insert into soci_test (id, b) values(:id, :b)", soci::use(id), soci::use(blob);

        soci::rowset< soci::row > rowSet = sql.prepare << "select id, b from soci_test";
        bool containedData = false;
        for (auto it = rowSet.begin(); it != rowSet.end(); ++it)
        {
            containedData = true;
            const soci::row &currentRow = *it;

            CHECK(currentRow.get_properties(1).get_data_type() == soci::dt_blob);
        }
        CHECK(containedData);
    }
    SECTION("Blob binding")
    {
        // Add data
        soci::blob blob1(sql);
        soci::blob blob2(sql);
        static_assert(10 <= sizeof(dummy_data), "Underlying assumption violated");
        blob1.write_from_start(dummy_data, 10);
        blob2.write_from_start(dummy_data, 10);
        const int id1 = 42;
        const int id2 = 42;
        sql << "insert into soci_test (id, b) values(:id, :b)", soci::use(id1), soci::use(blob1);
        sql << "insert into soci_test (id, b) values(:id, :b)", soci::use(id2), soci::use(blob2);

        SECTION("into")
        {
            soci::blob intoBlob(sql);

            sql << "select b from soci_test where id=:id", soci::use(id1), soci::into(intoBlob);

            char buffer[20];
            std::size_t written = intoBlob.read_from_start(buffer, sizeof(buffer));
            CHECK(written == 10);
            for (std::size_t i = 0; i < 10; ++i)
            {
                CHECK(buffer[i] == dummy_data[i]);
            }
        }
        SECTION("move_as")
        {
            soci::rowset< soci::row > rowSet = (sql.prepare << "select b from soci_test where id=:id", soci::use(id1));
            bool containedData = false;
            for (auto it = rowSet.begin(); it != rowSet.end(); ++it)
            {
                containedData = true;
                const soci::row &currentRow = *it;

                soci::blob intoBlob = currentRow.move_as<soci::blob>(0);

                CHECK(intoBlob.get_len() == 10);
                char buffer[20];
                std::size_t written = intoBlob.read_from_start(buffer, sizeof(buffer));
                CHECK(written == 10);
                for (std::size_t i = 0; i < 10; ++i)
                {
                    CHECK(buffer[i] == dummy_data[i]);
                }
            }
            CHECK(containedData);
        }
        SECTION("get into container")
        {
            soci::rowset< soci::row > rowSet = (sql.prepare << "select b from soci_test where id=:id", soci::use(id1));
            bool containedData = false;
            for (auto it = rowSet.begin(); it != rowSet.end(); ++it)
            {
                containedData = true;
                const soci::row &currentRow = *it;

                std::string strData = currentRow.get<std::string>(0);
                std::vector<unsigned char> vecData = currentRow.get<std::vector<unsigned char>>(0);

                // Container is required to hold a type that has a size of 1 byte
                CHECK_THROWS(currentRow.get<std::vector<int>>(0));
                // Using containers for which the soci::is_contiguous_resizable_container trait is not
                // defined yield a std::bad_cast exception.
                CHECK_THROWS(currentRow.get<std::list<char>>(0));
                CHECK_THROWS(currentRow.get<std::set<char>>(0));

                CHECK(strData.size() == 10);
                CHECK(vecData.size() == 10);
                for (std::size_t i = 0; i < 10; ++i)
                {
                    CHECK(strData[i] == dummy_data[i]);
                    CHECK(vecData[i] == dummy_data[i]);
                }
            }
            CHECK(containedData);
        }
        SECTION("reusing bound blob")
        {
            int secondID = id2 + 1;
            sql << "insert into soci_test(id, b) values(:id, :b)", soci::use(secondID), soci::use(blob2);

            // Selecting the blob associated with secondID should yield the same result as selecting the one for id
            soci::blob intoBlob(sql);
            sql << "select b from soci_test where id=:id", soci::use(secondID), soci::into(intoBlob);
            char buffer[20];
            std::size_t written = intoBlob.read_from_start(buffer, sizeof(buffer));
            CHECK(written == 10);
            for (std::size_t i = 0; i < 10; ++i)
            {
                CHECK(buffer[i] == dummy_data[i]);
            }
        }
    }
    SECTION("Statements")
    {
        unsigned int id;
        soci::blob myBlob(sql);
        soci::statement insert_stmt = (sql.prepare << "insert into soci_test (id, b) values (:id, :b)", soci::use(id), soci::use(myBlob));

        id = 1;
        myBlob.write_from_start(dummy_data + id, id);
        insert_stmt.execute(true);

        id = 5;
        myBlob.write_from_start(dummy_data + id, id);
        insert_stmt.execute(true);


        soci::statement select_stmt = (sql.prepare << "select id, b from soci_test order by id asc", soci::into(id), soci::into(myBlob));
        char contents[16];

        select_stmt.execute();
        CHECK(select_stmt.fetch());
        CHECK(id == 1);
        std::size_t blob_size = myBlob.get_len();
        CHECK(blob_size == id);
        std::size_t read = myBlob.read_from_start(contents, blob_size);
        CHECK(read == blob_size);
        for (unsigned int i = 0; i < blob_size; ++i)
        {
            CHECK(contents[i] == dummy_data[id + i]);
        }

        CHECK(select_stmt.fetch());
        CHECK(id == 5);
        blob_size = myBlob.get_len();
        CHECK(blob_size == id);
        read = myBlob.read_from_start(contents, blob_size);
        CHECK(read == blob_size);
        for (unsigned int i = 0; i < blob_size; ++i)
        {
            CHECK(contents[i] == dummy_data[id + i]);
        }

        CHECK(!select_stmt.fetch());
    }
}

} // namespace tests

} // namespace soci
