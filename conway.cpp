#include <array>
#include <cstdint>
#include <fstream>
#include <iostream>
#include <random>
#include <string>
#include <vector>
#include <cctype>
#include <cstdlib>

int cols = 91; // width, in increments of 2 characters
int rows = 64; // height, in lines
int density = 70; // initial density of tiles
int entropy = 10; // random factor, higher makes more random spawnings of tiles. it is necessary so that the game doesn't stabilize/go extinct.
int exclude_start_row = 25;
int exclude_end_row = 33;
int exclude_start_col = 40;
int exclude_end_col = 50;
bool hide_lonely_entropy_tiles = false; // lonely entropy tiles are tiles that were (1) randomly spawned in that frame and (2) have no neighbors, excluding those that were also randomly spawned in
bool exclude_enabled = true; // whether the exclusion zone is active. the exclusion zone was created for the purpose of not covering the password input box and clock.

std::vector<bool> entropy_flipped;

static std::string getConfigFilePath() {
    const char* home = getenv("HOME");
    if (home) {
        return std::string(home) + "/.config/hypr/conway_lockscreen/config.json";
    }
    return "./config.json";
}

constexpr const char* STATE_FILE = "/tmp/hyprlock_conway.state";

using Grid = std::vector<uint8_t>;

static std::mt19937 rng{std::random_device{}()};
static std::uniform_int_distribution<int> random100(0, 99);
static std::uniform_int_distribution<int> random1000(0, 999);

int jsonGetInt(const std::string& json, const std::string& key, int defaultValue) {
    size_t pos = json.find("\"" + key + "\":");
    if (pos == std::string::npos) {
        return defaultValue;
    }
    pos += key.length() + 3; // skip "{KEY}":
    while (pos < json.size() && std::isspace(json[pos])) {
        pos++;
    }
    bool negative = false;
    if (pos < json.size() && json[pos] == '-') {
        negative = true;
        pos++;
    }
    long val = 0;
    bool foundDigit = false;
    while (pos < json.size() && std::isdigit(json[pos])) {
        foundDigit = true;
        val = val * 10 + (json[pos] - '0');
        pos++;
    }
    if (!foundDigit) {
        return defaultValue;
    }
    if (negative) {
        val = -val;
    }
    return static_cast<int>(val);
}

bool jsonGetBool(const std::string& json, const std::string& key, bool defaultValue) {
    size_t pos = json.find("\"" + key + "\":");
    if (pos == std::string::npos) {
        return defaultValue;
    }
    pos += key.length() + 3; // skip "{KEY}":
    while (pos < json.size() && std::isspace(json[pos])) {
        pos++;
    }
    if (pos + 4 <= json.size() &&
        (json[pos] == 't' || json[pos] == 'T') &&
        (json[pos+1] == 'r' || json[pos+1] == 'R') &&
        (json[pos+2] == 'u' || json[pos+2] == 'U') &&
        (json[pos+3] == 'e' || json[pos+3] == 'E')) {
        return true;
    }
    if (pos + 5 <= json.size() &&
        (json[pos] == 'f' || json[pos] == 'F') &&
        (json[pos+1] == 'a' || json[pos+1] == 'A') &&
        (json[pos+2] == 'l' || json[pos+2] == 'L') &&
        (json[pos+3] == 's' || json[pos+3] == 'S') &&
        (json[pos+4] == 'e' || json[pos+4] == 'E')) {
        return false;
    }
    int intVal = jsonGetInt(json, key, -1);
    if (intVal == 0) {
        return false;
    } else if (intVal == 1) {
        return true;
    }
    return defaultValue;
}

void ensureEntropyFlippedSize() {
    if (rows <= 0 || cols <= 0) {
        entropy_flipped.clear();
        return;
    }
    size_t expected = static_cast<size_t>(rows) * static_cast<size_t>(cols);
    if (entropy_flipped.size() != expected) {
        entropy_flipped.assign(expected, false);
    }
}

void parseExclude(const std::string& json) {
    size_t excludePos = json.find("\"exclude\":");
    if (excludePos == std::string::npos) {
        return;
    }
    size_t bracePos = json.find('{', excludePos);
    if (bracePos == std::string::npos) {
        return;
    }
    int braceCount = 1;
    size_t i = bracePos + 1;
    while (i < json.size() && braceCount > 0) {
        if (json[i] == '{') braceCount++;
        else if (json[i] == '}') braceCount--;
        i++;
    }
    if (braceCount != 0) {
        return;
    }
    std::string excludeBlock = json.substr(bracePos + 1, i - bracePos - 2);

    exclude_start_row = jsonGetInt(excludeBlock, "start_row", exclude_start_row);
    exclude_end_row   = jsonGetInt(excludeBlock, "end_row",   exclude_end_row);
    exclude_start_col = jsonGetInt(excludeBlock, "start_col", exclude_start_col);
    exclude_end_col   = jsonGetInt(excludeBlock, "end_col",   exclude_end_col);
}

void loadConfig() {
    std::string configPath = getConfigFilePath();
    std::ifstream file(configPath);
    if (!file) {
        file.open("./config.json");
        if (!file) {
            std::cerr << "WARNING: config.json not found, using defaults\n";
            return;
        }
    }
    std::string json(
        (std::istreambuf_iterator<char>(file)),
        (std::istreambuf_iterator<char>())
    );

    int tmp = jsonGetInt(json, "columns", cols);
    if (tmp <= 0) {
        std::cerr << "WARNING: invalid columns, using default " << cols << "\n";
    } else {
        cols = tmp;
    }
    tmp = jsonGetInt(json, "rows", rows);
    if (tmp <= 0) {
        std::cerr << "WARNING: invalid rows, using default " << rows << "\n";
    } else {
        rows = tmp;
    }
    tmp = jsonGetInt(json, "density", density);
    if (tmp < 0 || tmp > 100) {
        std::cerr << "WARNING: invalid density, using default " << density << "\n";
    } else {
        density = tmp;
    }
    tmp = jsonGetInt(json, "entropy", entropy);
    if (tmp < 0) {
        std::cerr << "WARNING: invalid entropy, using default " << entropy << "\n";
    } else {
        entropy = tmp;
    }
    hide_lonely_entropy_tiles = jsonGetBool(json, "hide_lonely_entropy_tiles", hide_lonely_entropy_tiles);
    exclude_enabled = jsonGetBool(json, "exclude_enabled", exclude_enabled);
    parseExclude(json);

    entropy_flipped.assign(rows * cols, false);
}

Grid generateRandomGrid()
{
    Grid grid(rows * cols);
    for (auto& cell : grid)
        cell = random100(rng) < density;
    return grid;
}

bool loadGrid(Grid& grid)
{
    std::ifstream file(STATE_FILE, std::ios::binary);
    if (!file)
        return false;
    grid.resize(rows * cols);
    file.read(
        reinterpret_cast<char*>(grid.data()),
        static_cast<std::streamsize>(grid.size())
    );
    return file.gcount() == static_cast<std::streamsize>(grid.size());
}

void saveGrid(const Grid& grid)
{
    std::ofstream file(STATE_FILE, std::ios::binary | std::ios::trunc);
    if (!file)
        return;
    file.write(
        reinterpret_cast<const char*>(grid.data()),
        static_cast<std::streamsize>(grid.size())
    );
}

inline int wrapRow(int row)
{
    if (row < 0)
        return rows - 1;
    if (row >= rows)
        return 0;
    return row;
}

inline int wrapCol(int col)
{
    if (col < 0)
        return cols - 1;
    if (col >= cols)
        return 0;
    return col;
}

Grid nextGeneration(const Grid& grid)
{
    ensureEntropyFlippedSize();
    Grid next(rows * cols);

    for (int row = 0; row < rows; ++row) {
        const int up   = wrapRow(row - 1);
        const int down = wrapRow(row + 1);

        for (int col = 0; col < cols; ++col) {
            bool excluded = false;
            if (exclude_enabled) {
                if (row >= exclude_start_row &&
                    row <= exclude_end_row &&
                    col >= exclude_start_col &&
                    col <= exclude_end_col)
                {
                    excluded = true;
                }
            }

            if (excluded) {
                next[row * cols + col] = 0;
                entropy_flipped[row * cols + col] = false;
                continue;
            }

            const int left  = wrapCol(col - 1);
            const int right = wrapCol(col + 1);

            int neighbors =
                grid[up   * cols + left] +
                grid[up   * cols + col] +
                grid[up   * cols + right] +
                grid[row * cols + left] +
                grid[row * cols + right] +
                grid[down * cols + left] +
                grid[down * cols + col] +
                grid[down * cols + right];

            const uint8_t current = grid[row * cols + col];
            uint8_t nextCell = 0;

            if (current) {
                if (neighbors == 2 || neighbors == 3)
                    nextCell = 1;
            } else {
                if (neighbors == 3)
                    nextCell = 1;
            }

            bool flipped = false;
            if (entropy > 0 && random1000(rng) < entropy) {
                nextCell = !nextCell;
                flipped = true;
            }

            next[row * cols + col] = nextCell;
            entropy_flipped[row * cols + col] = flipped;
        }
    }

    return next;
}

void printGrid(const Grid& grid)
{
    ensureEntropyFlippedSize();
    std::string output;
    output.reserve(rows * (cols * 2 + 1));

    for (int row = 0; row < rows; ++row) {
        for (int col = 0; col < cols; ++col) {
            bool excluded = false;
            if (exclude_enabled) {
                if (row >= exclude_start_row &&
                    row <= exclude_end_row &&
                    col >= exclude_start_col &&
                    col <= exclude_end_col)
                {
                    excluded = true;
                }
            }

            if (excluded) {
                output += "  ";
                continue;
            }

            const size_t idx = row * cols + col;
            const bool entropyFlipped = entropy_flipped[idx];

            if (hide_lonely_entropy_tiles && entropyFlipped) {
                int liveNonEntropyNeighbors = 0;
                const int up   = wrapRow(row - 1);
                const int down = wrapRow(row + 1);
                const int left  = wrapCol(col - 1);
                const int right = wrapCol(col + 1);

                int nrow = up; int ncol = left;
                if (!entropy_flipped[nrow * cols + ncol] && grid[nrow * cols + ncol]) liveNonEntropyNeighbors++;
                nrow = up; ncol = col;
                if (!entropy_flipped[nrow * cols + ncol] && grid[nrow * cols + ncol]) liveNonEntropyNeighbors++;
                nrow = up; ncol = right;
                if (!entropy_flipped[nrow * cols + ncol] && grid[nrow * cols + ncol]) liveNonEntropyNeighbors++;
                nrow = row; ncol = left;
                if (!entropy_flipped[nrow * cols + ncol] && grid[nrow * cols + ncol]) liveNonEntropyNeighbors++;
                nrow = row; ncol = right;
                if (!entropy_flipped[nrow * cols + ncol] && grid[nrow * cols + ncol]) liveNonEntropyNeighbors++;
                nrow = down; ncol = left;
                if (!entropy_flipped[nrow * cols + ncol] && grid[nrow * cols + ncol]) liveNonEntropyNeighbors++;
                nrow = down; ncol = col;
                if (!entropy_flipped[nrow * cols + ncol] && grid[nrow * cols + ncol]) liveNonEntropyNeighbors++;
                nrow = down; ncol = right;
                if (!entropy_flipped[nrow * cols + ncol] && grid[nrow * cols + ncol]) liveNonEntropyNeighbors++;

                if (liveNonEntropyNeighbors == 0) {
                    output += "  ";
                    continue;
                }
            }

            if (!grid[idx]) {
                output += "  ";
            } else {
                output += "\xE2\x96\xA0 ";
            }
        }
        if (row != rows - 1)
            output += '\n';
    }

    std::cout << output;
}

int main()
{
    loadConfig();

    Grid grid;
    if (!loadGrid(grid))
        grid = generateRandomGrid();

    Grid next = nextGeneration(grid);
    saveGrid(next);
    printGrid(next);

    return 0;
}