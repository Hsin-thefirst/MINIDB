#include "MINIDB_SQL.h"

using namespace std;

int main(int argc, char* argv[])
{
    if (argc < 3) return 1; 
    string inputFilePath = argv[1];
    string outputFilePath = argv[2];
    string tempFilePath = "temp.txt";
    string tempFilePath2 = "temp2.txt";
    vector<string> prevLines = getInputLines(tempFilePath);
    bool ret = fileExists(tempFilePath);
    bool prev = false;
    if (ret)
    {
        for (int i = 0; i < prevLines.size(); i++)
            if (prevLines[i].find("SELECT") != string::npos) 
            {
                prevLines.erase(prevLines.begin() + i);
                i--;
            }
        prev = true;
        ofstream outFile(tempFilePath2);
        combineFiles(tempFilePath, inputFilePath, tempFilePath2);
        copyFile(tempFilePath2, tempFilePath);
        remove(tempFilePath2.c_str());
    }
    else
    {
        ofstream outFile(tempFilePath);
        if (outFile.is_open())
        {
            copyFile(inputFilePath, tempFilePath);
            outFile.close();
        }
    }
    vector<string> lines;
    vector<string> ttlines = getInputLines(inputFilePath);
    if (prev) for (string line : prevLines) lines.push_back(line);
    for (string line : ttlines) lines.push_back(line);
    operate(lines, outputFilePath);
    return 0; 
}
