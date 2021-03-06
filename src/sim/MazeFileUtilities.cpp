#include "MazeFileUtilities.h"

#include <fstream>
#include <iterator>

#include "Assert.h"
#include "Logging.h"
#include "SimUtilities.h"

namespace sim {

bool MazeFileUtilities::isMazeFile(const std::string& mazeFilePath) {

    // Definitions:
    //  X-value - The first integer value in a particular line
    //  Y-value - The second integer value in a particular line
    //  Column - A group of one or more lines that share the same X-value value
    //
    // Format requires that:
    //  - The file must exist
    //  - The file must not be empty
    //  - Each line consists of six whitespace separated tokens
    //  - Each of the six tokens are integer values
    //  - The last four tokens must be either 0 or 1
    //  - The lines should be sorted by X-value, and then by Y-value
    //  - The X-value of the first line should be 0
    //  - X-values should never decrease between any two subsequent lines
    //  - X-values should increase by at most 1 between any two subsequent lines
    //  - The Y-value of the first line of each column should be 0
    //  - Y-values should never decrease between any two subsequent lines within a column
    //  - Y-values should increase by at most 1 between any two subsequent lines
    //  - (X-value, Y-value) tuples must be unique
    //
    // Note that the maze does not have to be rectangular to be considered a maze file.

    // First, make sure we've been given a file
    if (!SimUtilities::isFile(mazeFilePath)) {
        L()->warn("\"%v\" is not a file.", mazeFilePath);
        return false;
    }

    // Create the file object
    std::ifstream file(mazeFilePath.c_str());

    // Error opening file
    if (!file.is_open()) {
        L()->warn("Could not open \"%v\" for maze validation.", mazeFilePath);
        return false;
    }

    // Empty file
    if (file.peek() == std::ifstream::traits_type::eof()) {
        L()->warn("\"%v\" is empty.", mazeFilePath);
        return false;
    }

    std::string line("");
    int lineNum = 0;
    int expectedX = 0;
    int expectedY = 0;

    while (std::getline(file, line)) {

        // Increment the line number
        lineNum += 1;

        // Extract the whitespace separated tokens
        std::vector<std::string> tokens = SimUtilities::tokenize(line);

        // Check to see that there are exactly six entries...
        if (6 != tokens.size()) {
            L()->warn(
                "\"%v\" does not contain six entries on each line: line %v contains %v entries.",
                mazeFilePath,
                std::to_string(lineNum),
                std::to_string(tokens.size()));
            return false;
        }

        // ... all of which are numeric
        std::vector<int> values;
        for (int i = 0; i < tokens.size(); i += 1) {
            if (!SimUtilities::isInt(tokens.at(i))) {
                L()->warn(
                    "\"%v\" contains non-numeric entries: the entry"
                    " \"%v\" on line %v in position %v is not numeric.",
                    mazeFilePath,
                    tokens.at(i),
                    std::to_string(lineNum),
                    std::to_string(i + 1));
                return false;
            }
            else {
                values.push_back(SimUtilities::strToInt(tokens.at(i)));
            }
        }

        // Check the expected X and expected Y. Note that the only time expect a Y-value of zero is the
        // very first line. The "&& expectedY != 0" ensures that the first line must be (0,0).
        bool caseOne = values.at(0) == expectedX     && values.at(1) == expectedY;
        bool caseTwo = values.at(0) == expectedX + 1 && values.at(1) == 0 && expectedY != 0;
        if (caseOne) {
            expectedY += 1;
        }
        else if (caseTwo) {
            expectedX += 1;
            expectedY = 1;
        }
        else {
            L()->warn(
                "\"%v\" contains unexpected x and y values of %v and %v on line %v.",
                mazeFilePath,
                std::to_string(values.at(0)),
                std::to_string(values.at(1)),
                std::to_string(lineNum));
            return false;
        }

        // Check the wall values to ensure that they're either 0 or 1
        for (int i = 0; i < 4; i += 1) {
            int value = values.at(2 + i);
            if (!(value == 0 || value == 1)) {
                L()->warn(
                    "\"%v\" contains an invalid value of %v in position %v on"
                    " line %v. All wall values must be either \"0\" or \"1\".",
                    mazeFilePath,
                    std::to_string(value),
                    std::to_string(2 + i + 1),
                    std::to_string(lineNum));
                return false;
            }
        }
    }

    return true;
}

void MazeFileUtilities::saveMaze(const std::vector<std::vector<BasicTile>>& maze, const std::string& mazeFilePath) {

    // Create the stream
    std::ofstream file(mazeFilePath.c_str());

    // Make sure the file is open
    if (!file.is_open()) {
        L()->warn("Unable to save maze to \"%v\".", mazeFilePath);
        return;
    }

    // Write to the file
    for (int x = 0; x < maze.size(); x += 1) {
        for (int y = 0; y < maze.at(x).size(); y += 1) {
            file << x << " " << y;
            for (Direction direction : DIRECTIONS) {
                file << " " << (maze.at(x).at(y).walls.at(direction) ? 1 : 0);
            }
            file << std::endl;
        }
    }

    file.close();
}

std::vector<std::vector<BasicTile>> MazeFileUtilities::loadMaze(const std::string& mazeFilePath) {

    // This should only be called on files that are actually maze files
    ASSERT_TR(MazeFileUtilities::isMazeFile(mazeFilePath));

    // The maze to be returned
    std::vector<std::vector<BasicTile>> maze;

    // The column to be appended
    std::vector<BasicTile> column;

    // Read the file and populate the wall values
    std::ifstream file(mazeFilePath.c_str());
    std::string line("");
    while (getline(file, line)) {

        // Put the tokens in a vector
        std::vector<std::string> tokens = SimUtilities::tokenize(line);

        // Fill the BasicTile object with the values
        BasicTile tile;
        for (Direction direction : DIRECTIONS) {
            tile.walls.insert(std::make_pair(direction,
                (1 == SimUtilities::strToInt(tokens.at(2 + SimUtilities::getDirectionIndex(direction))))));
        }

        // If the tile belongs to a new column, append the current column and then empty it
        if (maze.size() < SimUtilities::strToInt(tokens.at(0))) {
            maze.push_back(column);
            column.clear();
        }

        // Always append the current tile to the current column
        column.push_back(tile);
    }

    // Make sure to append the last column
    maze.push_back(column);

    // Remember to close the file
    file.close();
  
    return maze;
}

} //namespace sim
