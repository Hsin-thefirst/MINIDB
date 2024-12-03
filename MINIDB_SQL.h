#pragma once
#include <bits/stdc++.h>
using namespace std;

void trim(string &str)
{
    while (!str.empty() && str[0] == ' ')
    {
        str.erase(0, 1);
    }
    while (!str.empty() && str[str.size() - 1] == ' ')
    {
        str.pop_back();
    }
}

vector<string> getInputLines(const string &inputFilePath)
{
    ifstream input;
    vector<string> lines;
    input.open(inputFilePath);
    string line = "";
    string temp;
    while (getline(input, temp))
    {
        trim(temp);
        stringstream ss(temp);
        string word;
        while (ss >> word)
        {
            if (word.find(";") != string::npos)
            {
                word.pop_back();
                line += word;
                lines.push_back(line);
                line = "";
                continue;
            }
            line += word + " ";
        }
    }
    input.close();
    return lines;
}

class Row
{
public:
    vector<pair<string, string>> fields;
    // every field is a pair of name and columnName
    void addField(string name, string value)
    {
        fields.push_back(make_pair(name, value));
    }
};

class Table {
public:
    Table() : name("") {}
    Table(const string &Name) : name(Name) {}
    string name;
    Table* address;
    vector<pair<string, string>> columnNames;
    vector<Row> rows;

    void addRow(Row row) 
    {
        rows.push_back(row);
    }

    int getColumnIndex(const string& colName) const 
    {
        for (size_t i = 0; i < columnNames.size(); ++i) {
            if (columnNames[i].first == colName) {
                return i;
            }
        }
        return -1; // Column not found
    }

void print(const vector<string> &colNames, const string &outfilePath, bool append = false) const
{
    ofstream output;
    if (append)
    {
        output.open(outfilePath, ios_base::app);
        output << "---" << endl; // Separate query results
    }
    else
    {
        output.open(outfilePath);
    }

    // Print the header row
    for (size_t i = 0; i < colNames.size(); ++i)
    {
        output << colNames[i];
        if (i < colNames.size() - 1)
        {
            output << ",";
        }
    }
    output << endl;

    // Print the rows
    for (const auto &row : rows)
    {
        for (size_t i = 0; i < colNames.size(); ++i)
        {
            auto it = find_if(row.fields.begin(), row.fields.end(),
                              [&](const pair<string, string> &field)
                              {
                                  return field.second == colNames[i];
                              });
            if (it != row.fields.end())
            {
                const string &value = it->first;
                const string &columnName = it->second;
                auto colTypeIt = find_if(columnNames.begin(), columnNames.end(),
                                         [&](const pair<string, string> &colType)
                                         {
                                             return colType.first == columnName;
                                         });
                if (colTypeIt != columnNames.end())
                {
                    if (colTypeIt->second == "TEXT")
                    {
                        // Handle text fields, ensuring they're enclosed in double quotes
                        string processedValue = value;
                        if (!processedValue.empty() && (processedValue[0] == '\'' || processedValue[0] == '\"'))
                        {
                            processedValue.erase(0, 1);
                        }
                        if (!processedValue.empty() && (processedValue.back() == '\'' || processedValue.back() == '\"'))
                        {
                            processedValue.pop_back();
                        }
                        output << "\"" << processedValue << "\"";
                    }
                    else if (colTypeIt->second == "FLOAT")
                    {
                        float floatValue = stof(value);
                        output << fixed << setprecision(2) << floatValue;
                    }
                    else if (colTypeIt->second == "INTEGER")
                    {
                        output << value;
                    }
                    else
                    {
                        output << value;
                    }
                }
            }
            else
            {
                output << "NULL";
            }
            if (i < colNames.size() - 1)
            {
                output << ",";
            }
        }
        output << endl;
    }

    output.close(); // Ensure file is properly closed
}

};

class Database
{
public:
    Database() : name("") {}
    string name;
    vector<shared_ptr<Table>> tables;
    void setname(string Name)
    {
        this->name = Name;
    }
    shared_ptr<Table> copyTable(shared_ptr<Table> table)
    {
        auto newTable = make_shared<Table>(table->name);
        newTable->columnNames = table->columnNames;
        newTable->rows = table->rows;
        return newTable;
    }
    void createTable(const string &tableName, const vector<pair<string, string>> &columns)
    {
        auto table = make_shared<Table>(tableName);
        table->columnNames = columns;
        tables.push_back(table);
    }
    shared_ptr<Table> getTable(const string &tableName)
    {
        for (auto &table : tables)
        {
            if (table->name == tableName)
            {
                return table;
            }
        }
        return nullptr;
    }
};

vector<shared_ptr<Database>> databases;

shared_ptr<Table> where(shared_ptr<Database> DB, shared_ptr<Table> table, string condition)
{
    shared_ptr<Table> tempTable = DB->copyTable(table);
    tempTable->rows.clear(); // Clear the rows to add only matching records

    // Trim whitespace
    auto trim = [](string &str)
    {
        while (!str.empty() && str[0] == ' ')
            str.erase(0, 1);
        while (!str.empty() && str.back() == ' ')
            str.pop_back();
    };

    // Helper function to evaluate a single condition
    auto evaluateCondition = [&](const Row &row, const string &condition)
    {
        string column, op, value;
        size_t pos = 0;
        if ((pos = condition.find('>')) != string::npos)
        {
            op = ">";
        }
        else if ((pos = condition.find('<')) != string::npos)
        {
            op = "<";
        }
        else if ((pos = condition.find('=')) != string::npos)
        {
            op = "=";
        }

        column = condition.substr(0, pos);
        value = condition.substr(pos + 1);
        trim(column);
        trim(value);

        for (const auto &field : row.fields)
        {
            if (field.second == column)
            {
                if ((op == ">" && stod(field.first) > stod(value)) ||
                    (op == "<" && stod(field.first) < stod(value)) ||
                    (op == "=" && field.first == value))
                {
                    return true;
                }
            }
        }
        return false;
    };

    // Handle multiple conditions with AND/OR
    auto evaluateConditions = [&](const Row &row, const string &condition)
    {
        vector<string> andConditions, orConditions;
        size_t pos = 0, prev_pos = 0;
        bool isOr = false;
        
        // Separate conditions by AND and OR
        while ((pos = condition.find(" AND ", prev_pos)) != string::npos)
        {
            andConditions.push_back(condition.substr(prev_pos, pos - prev_pos));
            prev_pos = pos + 5;
        }
        andConditions.push_back(condition.substr(prev_pos));

        for (const auto &andCondition : andConditions)
        {
            pos = 0;
            prev_pos = 0;
            vector<string> orCond;

            while ((pos = andCondition.find(" OR ", prev_pos)) != string::npos)
            {
                orCond.push_back(andCondition.substr(prev_pos, pos - prev_pos));
                prev_pos = pos + 4;
                isOr = true;
            }
            orCond.push_back(andCondition.substr(prev_pos));

            if (isOr)
            {
                bool orResult = false;
                for (const auto &cond : orCond)
                {
                    orResult = orResult || evaluateCondition(row, cond);
                }
                if (!orResult)
                {
                    return false;
                }
            }
            else
            {
                if (!evaluateCondition(row, andCondition))
                {
                    return false;
                }
            }
        }
        return true;
    };

    // Evaluate the conditions
    for (const auto &row : table->rows)
    {
        if (evaluateConditions(row, condition))
        {
            tempTable->addRow(row);
        }
    }

    return tempTable;
}

string evaluateExpression(const string &expression, const map<string, string> &fieldValues, const string &dataType)
{
    stringstream ss(expression);
    double result = 0;
    string token;
    char operation = '+';

    while (ss >> token)
    {
        if (token == "+" || token == "-")
        {
            operation = token[0];
        }
        else
        {
            double value;
            try
            {
                value = stod(token);
            }
            catch (const invalid_argument &)
            {
                value = stod(fieldValues.at(token)); // Fetch the field value if it's not a number
            }

            if (operation == '+')
                result += value;
            else if (operation == '-')
                result -= value;
        }
    }

    // Convert result back to the appropriate data type
    if (dataType == "INT")
    {
        return to_string(static_cast<int>(result));
    }
    else if (dataType == "FLOAT")
    {
        return to_string(result);
    }
    else
    {
        throw invalid_argument("Unsupported data type for arithmetic operations.");
    }
}

void deleteRecords(shared_ptr<Table> table, const string &condition)
{
    // Trim whitespace
    auto trim = [](string &str)
    {
        while (!str.empty() && str[0] == ' ')
            str.erase(0, 1);
        while (!str.empty() && str.back() == ' ')
            str.pop_back();
    };

    // Helper function to evaluate a single condition
    auto evaluateCondition = [&](const Row &row, const string &condition)
    {
        string column, op, value;
        size_t pos = 0;
        if ((pos = condition.find('>')) != string::npos)
        {
            op = ">";
        }
        else if ((pos = condition.find('<')) != string::npos)
        {
            op = "<";
        }
        else if ((pos = condition.find('=')) != string::npos)
        {
            op = "=";
        }

        column = condition.substr(0, pos);
        value = condition.substr(pos + 1);
        trim(column);
        trim(value);

        for (const auto &field : row.fields)
        {
            if (field.second == column)
            {
                if ((op == ">" && stod(field.first) > stod(value)) ||
                    (op == "<" && stod(field.first) < stod(value)) ||
                    (op == "=" && field.first == value))
                {
                    return true;
                }
            }
        }
        return false;
    };

    // Handle multiple conditions with AND/OR
    auto evaluateConditions = [&](const Row &row, const string &condition)
    {
        vector<string> andConditions;
        size_t pos = 0, prev_pos = 0;

        // Separate conditions by AND
        while ((pos = condition.find(" AND ", prev_pos)) != string::npos)
        {
            andConditions.push_back(condition.substr(prev_pos, pos - prev_pos));
            prev_pos = pos + 5;
        }
        andConditions.push_back(condition.substr(prev_pos));

        for (const auto &andCondition : andConditions)
        {
            vector<string> orConditions;
            pos = 0;
            prev_pos = 0;

            // Separate conditions by OR
            while ((pos = andCondition.find(" OR ", prev_pos)) != string::npos)
            {
                orConditions.push_back(andCondition.substr(prev_pos, pos - prev_pos));
                prev_pos = pos + 4;
            }
            orConditions.push_back(andCondition.substr(prev_pos));

            bool orResult = false;
            for (const auto &orCondition : orConditions)
            {
                orResult = orResult || evaluateCondition(row, orCondition);
            }

            if (!orResult)
            {
                return false;
            }
        }
        return true;
    };

    // Remove rows based on the condition
    auto it = table->rows.begin();
    while (it != table->rows.end())
    {
        if (condition.empty() || evaluateConditions(*it, condition))
        {
            it = table->rows.erase(it);
        }
        else
        {
            ++it;
        }
    }
}

void copyFile(const string &inputFilePath, const string &outputFilePath)
{
    ifstream input(inputFilePath);
    ofstream output(outputFilePath);
    output << input.rdbuf();
    input.close();
    output.close();
}

void combineFiles(const string &inputFilePath1, const string &inputFilePath2, const string &outputFilePath)
{
    ifstream input1(inputFilePath1);
    ifstream input2(inputFilePath2);
    ofstream output(outputFilePath);
    output << input1.rdbuf();
    output << input2.rdbuf();
    input1.close();
    input2.close();
    output.close();
}

bool fileExists(const string &filePath)
{
    ifstream file(filePath);
    return file.good();
}

void reportError(const string &line, int lineNum)
{
    cout << "Syntax error in line: " << line << endl;
    cout << "Command line number: " << lineNum + 1 << endl;
}

void update(shared_ptr<Table> table, const vector<pair<string, string>> &updates, const string &condition)
{
    // Helper function to evaluate a single condition
    auto evaluateCondition = [&](const Row &row, const string &condition)
    {
        string column, op, value;
        size_t pos = 0;
        if ((pos = condition.find('>')) != string::npos)
        {
            op = ">";
        }
        else if ((pos = condition.find('<')) != string::npos)
        {
            op = "<";
        }
        else if ((pos = condition.find('=')) != string::npos)
        {
            op = "=";
        }

        column = condition.substr(0, pos);
        value = condition.substr(pos + 1);
        trim(column);
        trim(value);

        for (const auto &field : row.fields)
        {
            if (field.second == column)
            {
                if ((op == ">" && stod(field.first) > stod(value)) ||
                    (op == "<" && stod(field.first) < stod(value)) ||
                    (op == "=" && field.first == value))
                {
                    return true;
                }
            }
        }
        return false;
    };

    // Handle multiple conditions with AND/OR
    auto evaluateConditions = [&](const Row &row, const string &condition)
    {
        vector<string> andConditions;
        size_t pos = 0, prev_pos = 0;

        // Separate conditions by AND
        while ((pos = condition.find(" AND ", prev_pos)) != string::npos)
        {
            andConditions.push_back(condition.substr(prev_pos, pos - prev_pos));
            prev_pos = pos + 5;
        }
        andConditions.push_back(condition.substr(prev_pos));

        for (const auto &andCondition : andConditions)
        {
            vector<string> orConditions;
            pos = 0;
            prev_pos = 0;

            // Separate conditions by OR
            while ((pos = andCondition.find(" OR ", prev_pos)) != string::npos)
            {
                orConditions.push_back(andCondition.substr(prev_pos, pos - prev_pos));
                prev_pos = pos + 4;
            }
            orConditions.push_back(andCondition.substr(prev_pos));

            bool orResult = false;
            for (const auto &orCondition : orConditions)
            {
                orResult = orResult || evaluateCondition(row, orCondition);
            }

            if (!orResult)
            {
                return false;
            }
        }
        return true;
    };

    // Update the rows based on the condition
    for (auto &row : table->rows)
    {
        if (condition.empty() || evaluateConditions(row, condition))
        {
            map<string, string> fieldValues;
            map<string, string> dataTypes;
            for (const auto &col : table->columnNames)
            {
                dataTypes[col.first] = col.second;
            }

            for (const auto &field : row.fields)
            {
                fieldValues[field.second] = field.first;
            }

            for (const auto &update : updates)
            {
                for (auto &field : row.fields)
                {
                    if (field.second == update.first)
                    {
                        field.first = evaluateExpression(update.second, fieldValues, dataTypes[field.second]);
                    }
                }
            }
        }
    }
}

void operate(vector<string> lines, string outfilePath)
{
    shared_ptr<Database> currentDB = nullptr;
    shared_ptr<Table> currentTable = nullptr;
    bool firstprint = true;
    int lineNum = 0;
    for (string line : lines)
    {
        lineNum++;
        stringstream ss(line);
        string command;
        ss >> command;
        if (command == "CREATE")
        {
            string type;
            ss >> type;
            if (type == "DATABASE")
            {
                string DBname;
                ss >> DBname;
                Database *db = new Database();
                db->setname(DBname);
                databases.push_back(shared_ptr<Database>(db));
                continue;
            }
            else if (type == "TABLE")
            {
                string TabName, colName, colType, temp;
                ss >> TabName;
                vector<pair<string, string>> columns;
                ss >> temp;
                while (ss >> colName >> colType)
                {
                    if (colType.find(",") != string::npos)
                    {
                        colType.pop_back();
                    }
                    if (colName.find(")") != string::npos || colType.find(")") != string::npos)
                    {
                        break;
                    }
                    columns.emplace_back(colName, colType);
                }
                if (currentDB)
                {
                    currentDB->createTable(TabName, columns);
                }
                continue;
            }
            reportError(line, lineNum);
        }
        else if (command == "SELECT")
        {
            if (line.find("INNER") != string::npos)
            {
                string temp, target1, target2, target3, target4, tableName1, tableName2, judgecol, colName1, colName2;
                ss >> target1 >> target2 >> temp >> tableName1 >> temp >> temp >> tableName2 >> temp >> target3 >> temp >>target4;
                Table tempTab;
                if (target1.find(",") != string::npos) target1.pop_back();
                colName1 = target1.substr(target1.find(".")+1);
                colName2 = target2.substr(target2.find(".")+1);
                judgecol = target3.substr(target3.find(".")+1);
                currentTable = currentDB->getTable(tableName1);
                shared_ptr<Table> table2 = currentDB->getTable(tableName2);
                if (currentTable)
                {
                    for (const auto &col: currentTable->columnNames)
                        if (col.first == colName1)
                            tempTab.columnNames.push_back(make_pair(col.first, col.second));
                    for (const auto &col: table2->columnNames)
                        if (col.first == colName2)
                            tempTab.columnNames.push_back(make_pair(col.first, col.second));
                    vector<Row> targetRows;
                    for (const auto &row1: currentTable->rows)
                        for (const auto &row2: table2->rows)
                            for (const auto &field1: row1.fields)
                                for (const auto &field2: row2.fields)
                                {
                                    if (judgecol == field1.second && field1.first == field2.first && field2.second == judgecol)
                                        {
                                            Row temprow1, temprow2, targetRow;
                                            for (const auto &field: row1.fields)
                                                if (field.second == colName1)
                                                    temprow1.addField(field.first, field.second);
                                            for (const auto &field: row2.fields)
                                                if (field.second == colName2)
                                                    temprow2.addField(field.first, field.second);
                                            for (const auto &field1: temprow1.fields)
                                                for (const auto &field2: temprow2.fields)
                                                            {
                                                                targetRow.addField(field1.first, field1.second);
                                                                targetRow.addField(field2.first, field2.second);
                                                            }
                                            targetRows.push_back(targetRow);
                                        }
                                }
                    for (Row c : targetRows) tempTab.addRow(c);
                    vector<string> colNames;
                    for (const auto &col : tempTab.columnNames)
                    {
                        colNames.push_back(col.first);
                    }
                    tempTab.print(colNames, outfilePath, !firstprint);
                    firstprint = false; 
                }
                continue;
            }
            string temp;
            vector<string> colNames;
            bool all = false;
            while (ss >> temp)
            {
                if (temp == "*")
                {
                    all = true;
                    ss >> temp;
                    break;
                }
                if (temp.find("FROM") != string::npos)
                {
                    break;
                }
                if (!all)
                {
                    if (temp.find(",") != string::npos)
                    {
                        temp.pop_back();
                    }
                    colNames.push_back(temp);
                }
            }
            string tableName;
            ss >> tableName;
            currentTable = currentDB->getTable(tableName);
            string condition;
            if (ss >> temp && temp == "WHERE")
            {
                getline(ss, condition);
                trim(condition);
            }

            if (currentTable)
            {
                if (!condition.empty())
                {
                    // Apply WHERE condition
                    currentTable = where(currentDB, currentTable, condition);
                }

                if (all)
                {
                    colNames.clear();
                    for (const auto &col : currentTable->columnNames)
                    {
                        colNames.push_back(col.first);
                    }
                }
                currentTable->print(colNames, outfilePath, !firstprint);
                firstprint = false;
                continue;
            }
            else
            {
                reportError(line, lineNum);
                continue;
            }
        }
        else if (command == "INSERT")
        {
            string temp, tableName;
            ss >> temp >> tableName; // Read "INTO" and table name
            ss >> temp; // Should be "VALUES"

            // Read the entire line for values
            string valuesLine;
            getline(ss, valuesLine);
            
            // Remove leading and trailing parentheses
            valuesLine.erase(0, valuesLine.find_first_not_of(" ("));
            valuesLine.erase(valuesLine.find_last_not_of(" )") + 1);

            // Split values by comma, respecting whitespace within quotes
            vector<string> values;
            stringstream valuesStream(valuesLine);
            string value;
            while (getline(valuesStream, value, ','))
            {
                trim(value);
                if (!value.empty() && value[0] == '\'')
                {
                    // Handle value enclosed in single quotes
                    if (value.back() == '\'')
                    {
                        value = value.substr(1, value.size() - 2);
                    }
                    else
                    {
                        string temp;
                        while (getline(valuesStream, temp, ','))
                        {
                            value += "," + temp;
                            if (temp.back() == '\'')
                            {
                                value = value.substr(1, value.size() - 2);
                                break;
                            }
                        }
                    }
                }
                values.push_back(value);
            }

            currentTable = currentDB->getTable(tableName);
            if (currentTable)
            {
                if (values.size() == currentTable->columnNames.size())
                {
                    Row row;
                    for (size_t i = 0; i < values.size(); ++i)
                    {
                        row.addField(values[i], currentTable->columnNames[i].first);
                    }
                    currentTable->addRow(row);
                }
                else
                {
                    cout << "Column count doesn't match value count for table: " << tableName << endl;
                    reportError(line, lineNum);
                }
            }
            else
            {
                reportError(line, lineNum);
                continue;
            }
        }
        else if (command == "USE")
        {
            string DBname, temp;
            ss >> temp >> DBname;
            bool found = false;
            for (auto &db : databases)
            {
                if (db->name == DBname)
                {
                    currentDB = db;
                    found = true;
                    break;
                }
            }
            if (!found)
            {
                cout << "Database not found: " << DBname << endl;
                reportError(line, lineNum);
            }
        }
        else if (command == "DROP")
        {
            string type, tabname;
            ss >> type >> tabname;
            bool found = false;
            for (auto &table : currentDB->tables)
            {
                if (table->name == tabname)
                {
                    table = nullptr;
                    found = true;
                    break;
                }
            }
            if (!found)
            {
                cout << "Table not found: " << tabname << endl;
                reportError(line, lineNum);
            }
        }
        else if (command == "UPDATE")
        {
            string tableName, temp;
            ss >> tableName >> temp; // temp should be "SET"
            string setClause;
            getline(ss, setClause); // Read the entire line

            // Find WHERE clause position
            size_t wherePos = setClause.find(" WHERE ");
            string condition;
            if (wherePos != string::npos)
            {
                condition = setClause.substr(wherePos + 7); // Extract condition after " WHERE "
                setClause = setClause.substr(0, wherePos); // Extract SET clause
            }

            vector<pair<string, string>> updates;
            stringstream setStream(setClause);
            while (getline(setStream, temp, ','))
            {
                size_t equalPos = temp.find('=');
                if (equalPos != string::npos)
                {
                    string column = temp.substr(0, equalPos);
                    string value = temp.substr(equalPos + 1);
                    trim(column);
                    trim(value);
                    updates.emplace_back(column, value);
                }
            }

            if (currentDB)
            {
                currentTable = currentDB->getTable(tableName);
                if (currentTable)
                {
                    // Apply updates
                    update(currentTable, updates, condition);
                }
                else
                {
                    cout << "Table not found: " << tableName << endl;
                    reportError(line, lineNum);
                }
            }
            else
            {
                cout << "No current database selected." << endl;
                reportError(line, lineNum);
            }
        }
        else if (command == "DELETE")
        {
            string temp, tableName;
            ss >> temp >> tableName;

            // Handle WHERE clause if present
            string condition;
            if (ss >> temp && temp == "WHERE")
            {
                getline(ss, condition);
                trim(condition);
            }

            if (currentDB)
            {
                currentTable = currentDB->getTable(tableName);
                if (currentTable)
                {
                    // Delete records based on the condition
                    deleteRecords(currentTable, condition);
                }
                else
                {
                    cout << "Table not found: " << tableName << endl;
                    reportError(line, lineNum);
                }
            }
            else
            {
                cout << "No current database selected." << endl;
                reportError(line, lineNum);
            }
        }
        else
        {
            cout << "Invalid command: " << command << endl;
            reportError(line, lineNum);
        }
    }
}
