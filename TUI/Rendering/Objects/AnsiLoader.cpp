#include "Rendering/Objects/AnsiLoader.h"

#include <algorithm>
#include <cctype>
#include <cstdint>
#include <fstream>
#include <iterator>
#include <map>
#include <optional>
#include <sstream>
#include <string>
#include <utility>
#include <vector>

#include "Rendering/Styles/Color.h"
#include "Utilities/Unicode/UnicodeConversion.h"
#include "Utilities/Unicode/UnicodeWidth.h"

namespace
{
    std::string toLowerCopy(std::string value)
    {
        std::transform(
            value.begin(),
            value.end(),
            value.begin(),
            [](unsigned char ch)
            {
                return static_cast<char>(std::tolower(ch));
            });

        return value;
    }

    std::string getFileExtensionLower(const std::string& filePath)
    {
        const std::size_t dot = filePath.find_last_of('.');
        if (dot == std::string::npos)
        {
            return {};
        }

        return toLowerCopy(filePath.substr(dot));
    }

    bool readAllBytes(const std::string& filePath, std::string& outBytes)
    {
        std::ifstream file(filePath, std::ios::binary);
        if (!file)
        {
            return false;
        }

        outBytes.assign(
            std::istreambuf_iterator<char>(file),
            std::istreambuf_iterator<char>());

        return true;
    }

    char32_t decodeCp437Byte(unsigned char value)
    {
        if (value <= 0x7F)
        {
            return UnicodeConversion::sanitizeCodePoint(static_cast<char32_t>(value));
        }

        static const char32_t kCp437Table[128] =
        {
            U'\u00C7', U'\u00FC', U'\u00E9', U'\u00E2', U'\u00E4', U'\u00E0', U'\u00E5', U'\u00E7',
            U'\u00EA', U'\u00EB', U'\u00E8', U'\u00EF', U'\u00EE', U'\u00EC', U'\u00C4', U'\u00C5',
            U'\u00C9', U'\u00E6', U'\u00C6', U'\u00F4', U'\u00F6', U'\u00F2', U'\u00FB', U'\u00F9',
            U'\u00FF', U'\u00D6', U'\u00DC', U'\u00A2', U'\u00A3', U'\u00A5', U'\u20A7', U'\u0192',
            U'\u00E1', U'\u00ED', U'\u00F3', U'\u00FA', U'\u00F1', U'\u00D1', U'\u00AA', U'\u00BA',
            U'\u00BF', U'\u2310', U'\u00AC', U'\u00BD', U'\u00BC', U'\u00A1', U'\u00AB', U'\u00BB',
            U'\u2591', U'\u2592', U'\u2593', U'\u2502', U'\u2524', U'\u2561', U'\u2562', U'\u2556',
            U'\u2555', U'\u2563', U'\u2551', U'\u2557', U'\u255D', U'\u255C', U'\u255B', U'\u2510',
            U'\u2514', U'\u2534', U'\u252C', U'\u251C', U'\u2500', U'\u253C', U'\u255E', U'\u255F',
            U'\u255A', U'\u2554', U'\u2569', U'\u2566', U'\u2560', U'\u2550', U'\u256C', U'\u2567',
            U'\u2568', U'\u2564', U'\u2565', U'\u2559', U'\u2558', U'\u2552', U'\u2553', U'\u256B',
            U'\u256A', U'\u2518', U'\u250C', U'\u2588', U'\u2584', U'\u258C', U'\u2590', U'\u2580',
            U'\u03B1', U'\u00DF', U'\u0393', U'\u03C0', U'\u03A3', U'\u03C3', U'\u00B5', U'\u03C4',
            U'\u03A6', U'\u0398', U'\u03A9', U'\u03B4', U'\u221E', U'\u03C6', U'\u03B5', U'\u2229',
            U'\u2261', U'\u00B1', U'\u2265', U'\u2264', U'\u2320', U'\u2321', U'\u00F7', U'\u2248',
            U'\u00B0', U'\u2219', U'\u00B7', U'\u221A', U'\u207F', U'\u00B2', U'\u25A0', U'\u00A0'
        };

        return UnicodeConversion::sanitizeCodePoint(kCp437Table[value - 0x80]);
    }

    Color::Basic ansiBasicToColor(int ansiCode)
    {
        switch (ansiCode)
        {
        case 0: return Color::Basic::Black;
        case 1: return Color::Basic::Red;
        case 2: return Color::Basic::Green;
        case 3: return Color::Basic::Yellow;
        case 4: return Color::Basic::Blue;
        case 5: return Color::Basic::Magenta;
        case 6: return Color::Basic::Cyan;
        case 7: return Color::Basic::White;
        default: return Color::Basic::White;
        }
    }

    Color::Basic ansiBrightBasicToColor(int ansiCode)
    {
        switch (ansiCode)
        {
        case 0: return Color::Basic::BrightBlack;
        case 1: return Color::Basic::BrightRed;
        case 2: return Color::Basic::BrightGreen;
        case 3: return Color::Basic::BrightYellow;
        case 4: return Color::Basic::BrightBlue;
        case 5: return Color::Basic::BrightMagenta;
        case 6: return Color::Basic::BrightCyan;
        case 7: return Color::Basic::BrightWhite;
        default: return Color::Basic::BrightWhite;
        }
    }

    struct MutableCell
    {
        char32_t glyph = U' ';
        CellKind kind = CellKind::Glyph;
        CellWidth width = CellWidth::One;
        std::optional<Style> style;
    };

    class GridBuilder
    {
    public:
        GridBuilder(
            int defaultColumns,
            int maxColumns,
            int maxRows,
            bool preserveTrailingSpaces)
            : m_defaultColumns(std::max(1, defaultColumns))
            , m_maxColumns(std::max(1, maxColumns))
            , m_maxRows(std::max(1, maxRows))
            , m_preserveTrailingSpaces(preserveTrailingSpaces)
        {
            m_width = m_defaultColumns;
            m_height = 1;
        }

        bool setCell(
            int x,
            int y,
            char32_t glyph,
            const std::optional<Style>& style,
            std::vector<AnsiLoader::LoadWarning>& warnings)
        {
            if (x < 0 || y < 0)
            {
                return false;
            }

            if (x >= m_maxColumns || y >= m_maxRows)
            {
                return false;
            }

            if (x + 1 > m_width)
            {
                m_width = x + 1;
            }

            if (y + 1 > m_height)
            {
                m_height = y + 1;
            }

            if (x >= m_defaultColumns)
            {
                warnings.push_back({
                    AnsiLoader::LoadWarningCode::CanvasExpanded,
                    "ANSI content exceeded default column width; canvas was expanded."
                    });
            }

            MutableCell cell;
            cell.glyph = UnicodeConversion::sanitizeCodePoint(glyph);
            cell.kind = CellKind::Glyph;
            cell.width = CellWidth::One;
            cell.style = style;

            m_cells[std::make_pair(x, y)] = cell;
            return true;
        }

        TextObject buildTextObject() const
        {
            const int finalWidth = std::max(1, m_width);
            const int finalHeight = std::max(1, m_height);

            std::u32string blank;
            blank.reserve(static_cast<std::size_t>(finalWidth * finalHeight + finalHeight));

            for (int y = 0; y < finalHeight; ++y)
            {
                for (int x = 0; x < finalWidth; ++x)
                {
                    blank.push_back(U' ');
                }

                if (y + 1 < finalHeight)
                {
                    blank.push_back(U'\n');
                }
            }

            TextObject object = TextObject::fromU32(blank);

            for (int y = 0; y < finalHeight; ++y)
            {
                for (int x = 0; x < finalWidth; ++x)
                {
                    TextObjectCell& outCell =
                        const_cast<TextObjectCell&>(object.getCell(x, y));

                    outCell.glyph = U' ';
                    outCell.kind = CellKind::Glyph;
                    outCell.width = CellWidth::One;
                    outCell.style.reset();
                }
            }

            for (const auto& entry : m_cells)
            {
                const int x = entry.first.first;
                const int y = entry.first.second;
                const MutableCell& source = entry.second;

                TextObjectCell& outCell =
                    const_cast<TextObjectCell&>(object.getCell(x, y));

                outCell.glyph = source.glyph;
                outCell.kind = source.kind;
                outCell.width = source.width;
                outCell.style = source.style;
            }

            return object;
        }

        int width() const
        {
            return std::max(1, m_width);
        }

        int height() const
        {
            return std::max(1, m_height);
        }

    private:
        int m_defaultColumns = 80;
        int m_maxColumns = 4096;
        int m_maxRows = 4096;
        bool m_preserveTrailingSpaces = true;

        int m_width = 80;
        int m_height = 1;

        std::map<std::pair<int, int>, MutableCell> m_cells;
    };

    class Parser
    {
    public:
        explicit Parser(const AnsiLoader::LoadOptions& options)
            : m_options(options)
            , m_grid(
                options.defaultColumns,
                options.maxColumns,
                options.maxRows,
                options.preserveTrailingSpaces)
        {
            m_currentStyle = options.baseStyle;
            m_savedStyle = m_currentStyle;
        }

        AnsiLoader::LoadResult parse(std::string_view bytes)
        {
            for (std::size_t i = 0; i < bytes.size(); ++i)
            {
                const unsigned char byte = static_cast<unsigned char>(bytes[i]);

                if (byte == 0x1B)
                {
                    if (!consumeEscape(bytes, i))
                    {
                        return fail("Malformed or unsupported ANSI escape sequence.");
                    }

                    continue;
                }

                if (byte == '\r')
                {
                    m_cursorX = 0;
                    continue;
                }

                if (byte == '\n')
                {
                    if (m_options.treatBareLfAsNewLine)
                    {
                        ++m_cursorY;
                    }
                    continue;
                }

                if (byte == '\b')
                {
                    m_cursorX = std::max(0, m_cursorX - 1);
                    continue;
                }

                if (byte == '\t')
                {
                    const int nextTabStop = ((m_cursorX / 8) + 1) * 8;
                    m_cursorX = nextTabStop;
                    clampCursorIfRequested();
                    continue;
                }

                const char32_t glyph = decodeCp437Byte(byte);
                if (!writeGlyph(glyph))
                {
                    return fail("ANSI output exceeded loader canvas limits.");
                }
            }

            AnsiLoader::LoadResult result;
            result.object = m_grid.buildTextObject();
            result.success = true;
            result.detectedFileType = AnsiLoader::FileType::Ans;
            result.resolvedWidth = m_grid.width();
            result.resolvedHeight = m_grid.height();
            result.warnings = m_warnings;
            return result;
        }

    private:
        bool consumeEscape(std::string_view bytes, std::size_t& ioIndex)
        {
            if (ioIndex + 1 >= bytes.size())
            {
                m_warnings.push_back({
                    AnsiLoader::LoadWarningCode::BareEscapeIgnored,
                    "Trailing ESC byte was ignored."
                    });

                if (m_options.strictUnsupportedCommands)
                {
                    return false;
                }

                return true;
            }

            const unsigned char next = static_cast<unsigned char>(bytes[ioIndex + 1]);
            if (next != '[')
            {
                m_warnings.push_back({
                    AnsiLoader::LoadWarningCode::UnsupportedSequenceIgnored,
                    "Non-CSI ESC sequence was ignored."
                    });

                if (m_options.strictUnsupportedCommands)
                {
                    return false;
                }

                ++ioIndex;
                return true;
            }

            std::size_t cursor = ioIndex + 2;
            std::string payload;

            while (cursor < bytes.size())
            {
                const unsigned char ch = static_cast<unsigned char>(bytes[cursor]);
                if (ch >= 0x40 && ch <= 0x7E)
                {
                    const char finalByte = static_cast<char>(ch);
                    ioIndex = cursor;
                    return applyCsi(payload, finalByte);
                }

                payload.push_back(static_cast<char>(ch));
                ++cursor;
            }

            return false;
        }

        bool applyCsi(const std::string& payload, char finalByte)
        {
            bool privateMode = false;
            std::string body = payload;

            if (!body.empty() && (body[0] == '?' || body[0] == '>' || body[0] == '='))
            {
                privateMode = true;
                body.erase(body.begin());
            }

            if (privateMode)
            {
                m_warnings.push_back({
                    AnsiLoader::LoadWarningCode::UnsupportedPrivateSequenceIgnored,
                    "ANSI private CSI sequence was ignored."
                    });

                return !m_options.strictUnsupportedCommands;
            }

            const std::vector<int> params = parseParams(body);

            switch (finalByte)
            {
            case 'm':
                return applySgr(params);

            case 'A':
                moveRelative(0, -firstOr(params, 1));
                return true;

            case 'B':
                moveRelative(0, firstOr(params, 1));
                return true;

            case 'C':
                moveRelative(firstOr(params, 1), 0);
                return true;

            case 'D':
                moveRelative(-firstOr(params, 1), 0);
                return true;

            case 'H':
            case 'f':
                moveAbsolute(firstOr(params, 1) - 1, getParam(params, 1, 1) - 1);
                return true;

            case 'G':
                moveAbsolute(m_cursorY, firstOr(params, 1) - 1);
                return true;

            case 'J':
                return eraseDisplay(firstOr(params, 0));

            case 'K':
                return eraseLine(firstOr(params, 0));

            case 's':
                m_savedCursorX = m_cursorX;
                m_savedCursorY = m_cursorY;
                m_savedStyle = m_currentStyle;
                return true;

            case 'u':
                m_cursorX = m_savedCursorX;
                m_cursorY = m_savedCursorY;
                m_currentStyle = m_savedStyle;
                clampCursorIfRequested();
                return true;

            default:
                m_warnings.push_back({
                    AnsiLoader::LoadWarningCode::UnsupportedSequenceIgnored,
                    std::string("Unsupported CSI final byte ignored: ") + finalByte
                    });

                return !m_options.strictUnsupportedCommands;
            }
        }

        std::vector<int> parseParams(const std::string& body) const
        {
            if (body.empty())
            {
                return {};
            }

            std::vector<int> params;
            std::string current;

            for (char ch : body)
            {
                if (ch == ';')
                {
                    params.push_back(current.empty() ? 0 : std::stoi(current));
                    current.clear();
                    continue;
                }

                if (std::isdigit(static_cast<unsigned char>(ch)))
                {
                    current.push_back(ch);
                    continue;
                }

                params.push_back(0);
                current.clear();
            }

            params.push_back(current.empty() ? 0 : std::stoi(current));
            return params;
        }

        int firstOr(const std::vector<int>& params, int fallback) const
        {
            return params.empty() ? fallback : (params[0] == 0 ? fallback : params[0]);
        }

        int getParam(const std::vector<int>& params, std::size_t index, int fallback) const
        {
            if (index >= params.size())
            {
                return fallback;
            }

            return params[index] == 0 ? fallback : params[index];
        }

        void moveRelative(int dx, int dy)
        {
            m_cursorX += dx;
            m_cursorY += dy;
            clampCursorIfRequested();
        }

        void moveAbsolute(int row, int column)
        {
            m_cursorY = std::max(0, row);
            m_cursorX = std::max(0, column);
            clampCursorIfRequested();
        }

        void clampCursorIfRequested()
        {
            if (!m_options.clampCursorToCanvas)
            {
                return;
            }

            const int oldX = m_cursorX;
            const int oldY = m_cursorY;

            m_cursorX = std::clamp(m_cursorX, 0, std::max(0, m_options.maxColumns - 1));
            m_cursorY = std::clamp(m_cursorY, 0, std::max(0, m_options.maxRows - 1));

            if (m_cursorX != oldX || m_cursorY != oldY)
            {
                m_warnings.push_back({
                    AnsiLoader::LoadWarningCode::CursorClamped,
                    "Cursor movement exceeded configured bounds and was clamped."
                    });
            }
        }

        bool eraseDisplay(int mode)
        {
            // Practical subset:
            // 0 = clear from cursor to end of canvas region already touched
            // 1 = clear from start to cursor
            // 2 = clear full touched canvas
            // Since TextObject is built from internal cell data, we simply write styled spaces.
            const int width = std::max(m_grid.width(), m_options.defaultColumns);
            const int height = std::max(m_grid.height(), m_cursorY + 1);

            if (mode == 2)
            {
                for (int y = 0; y < height; ++y)
                {
                    for (int x = 0; x < width; ++x)
                    {
                        if (!m_grid.setCell(x, y, U' ', m_currentStyle, m_warnings))
                        {
                            return false;
                        }
                    }
                }

                return true;
            }

            if (mode == 1)
            {
                for (int y = 0; y <= m_cursorY; ++y)
                {
                    const int rowEnd = (y == m_cursorY) ? m_cursorX : width;
                    for (int x = 0; x < rowEnd; ++x)
                    {
                        if (!m_grid.setCell(x, y, U' ', m_currentStyle, m_warnings))
                        {
                            return false;
                        }
                    }
                }

                return true;
            }

            for (int y = m_cursorY; y < height; ++y)
            {
                const int rowStart = (y == m_cursorY) ? m_cursorX : 0;
                for (int x = rowStart; x < width; ++x)
                {
                    if (!m_grid.setCell(x, y, U' ', m_currentStyle, m_warnings))
                    {
                        return false;
                    }
                }
            }

            return true;
        }

        bool eraseLine(int mode)
        {
            const int width = std::max(m_grid.width(), m_options.defaultColumns);

            if (mode == 2)
            {
                for (int x = 0; x < width; ++x)
                {
                    if (!m_grid.setCell(x, m_cursorY, U' ', m_currentStyle, m_warnings))
                    {
                        return false;
                    }
                }

                return true;
            }

            if (mode == 1)
            {
                for (int x = 0; x <= m_cursorX; ++x)
                {
                    if (!m_grid.setCell(x, m_cursorY, U' ', m_currentStyle, m_warnings))
                    {
                        return false;
                    }
                }

                return true;
            }

            for (int x = m_cursorX; x < width; ++x)
            {
                if (!m_grid.setCell(x, m_cursorY, U' ', m_currentStyle, m_warnings))
                {
                    return false;
                }
            }

            return true;
        }

        bool applySgr(const std::vector<int>& params)
        {
            if (params.empty())
            {
                m_currentStyle = m_options.baseStyle;
                return true;
            }

            for (std::size_t i = 0; i < params.size(); ++i)
            {
                const int p = params[i];

                switch (p)
                {
                case 0:
                    m_currentStyle = m_options.baseStyle;
                    break;

                case 1:
                    m_currentStyle = resolveStyle().withBold(true);
                    break;

                case 2:
                    m_currentStyle = resolveStyle().withDim(true);
                    break;

                case 4:
                    m_currentStyle = resolveStyle().withUnderline(true);
                    break;

                case 5:
                    m_currentStyle = resolveStyle().withSlowBlink(true);
                    break;

                case 6:
                    m_currentStyle = resolveStyle().withFastBlink(true);
                    break;

                case 7:
                    m_currentStyle = resolveStyle().withReverse(true);
                    break;

                case 8:
                    m_currentStyle = resolveStyle().withInvisible(true);
                    break;

                case 9:
                    m_currentStyle = resolveStyle().withStrike(true);
                    break;

                case 22:
                    m_currentStyle = resolveStyle().withoutBold().withoutDim();
                    break;

                case 24:
                    m_currentStyle = resolveStyle().withoutUnderline();
                    break;

                case 25:
                    m_currentStyle = resolveStyle().withoutSlowBlink().withoutFastBlink();
                    break;

                case 27:
                    m_currentStyle = resolveStyle().withoutReverse();
                    break;

                case 28:
                    m_currentStyle = resolveStyle().withoutInvisible();
                    break;

                case 29:
                    m_currentStyle = resolveStyle().withoutStrike();
                    break;

                case 39:
                    m_currentStyle = resolveStyle().withoutForeground();
                    break;

                case 49:
                    m_currentStyle = resolveStyle().withoutBackground();
                    break;

                default:
                    if (p >= 30 && p <= 37)
                    {
                        m_currentStyle = resolveStyle().withForeground(
                            Color::FromBasic(ansiBasicToColor(p - 30)));
                    }
                    else if (p >= 40 && p <= 47)
                    {
                        m_currentStyle = resolveStyle().withBackground(
                            Color::FromBasic(ansiBasicToColor(p - 40)));
                    }
                    else if (p >= 90 && p <= 97)
                    {
                        m_currentStyle = resolveStyle().withForeground(
                            Color::FromBasic(ansiBrightBasicToColor(p - 90)));
                    }
                    else if (p >= 100 && p <= 107)
                    {
                        m_currentStyle = resolveStyle().withBackground(
                            Color::FromBasic(ansiBrightBasicToColor(p - 100)));
                    }
                    else if (p == 38 || p == 48)
                    {
                        const bool foreground = (p == 38);
                        if (!applyExtendedColor(params, i, foreground))
                        {
                            m_warnings.push_back({
                                AnsiLoader::LoadWarningCode::UnsupportedSequenceIgnored,
                                "Malformed extended SGR color sequence ignored."
                                });

                            if (m_options.strictUnsupportedCommands)
                            {
                                return false;
                            }
                        }
                    }
                    else
                    {
                        m_warnings.push_back({
                            AnsiLoader::LoadWarningCode::UnsupportedSequenceIgnored,
                            "Unsupported SGR attribute ignored."
                            });

                        if (m_options.strictUnsupportedCommands)
                        {
                            return false;
                        }
                    }

                    break;
                }
            }

            return true;
        }

        bool applyExtendedColor(const std::vector<int>& params, std::size_t& ioIndex, bool foreground)
        {
            if (ioIndex + 1 >= params.size())
            {
                return false;
            }

            const int mode = params[++ioIndex];

            if (mode == 5)
            {
                if (ioIndex + 1 >= params.size())
                {
                    return false;
                }

                const int value = params[++ioIndex];
                if (value < 0 || value > 255)
                {
                    return false;
                }

                if (foreground)
                {
                    m_currentStyle = resolveStyle().withForeground(
                        Color::FromIndexed(static_cast<std::uint8_t>(value)));
                }
                else
                {
                    m_currentStyle = resolveStyle().withBackground(
                        Color::FromIndexed(static_cast<std::uint8_t>(value)));
                }

                return true;
            }

            if (mode == 2)
            {
                if (ioIndex + 3 >= params.size())
                {
                    return false;
                }

                const int r = params[++ioIndex];
                const int g = params[++ioIndex];
                const int b = params[++ioIndex];

                if (r < 0 || r > 255 || g < 0 || g > 255 || b < 0 || b > 255)
                {
                    return false;
                }

                if (foreground)
                {
                    m_currentStyle = resolveStyle().withForeground(
                        Color::FromRgb(
                            static_cast<std::uint8_t>(r),
                            static_cast<std::uint8_t>(g),
                            static_cast<std::uint8_t>(b)));
                }
                else
                {
                    m_currentStyle = resolveStyle().withBackground(
                        Color::FromRgb(
                            static_cast<std::uint8_t>(r),
                            static_cast<std::uint8_t>(g),
                            static_cast<std::uint8_t>(b)));
                }

                return true;
            }

            return false;
        }

        Style resolveStyle() const
        {
            return m_currentStyle.value_or(Style{});
        }

        bool writeGlyph(char32_t glyph)
        {
            glyph = UnicodeConversion::sanitizeCodePoint(glyph);

            const CellWidth measuredWidth = UnicodeWidth::measureCodePointWidth(glyph);
            if (measuredWidth == CellWidth::Zero)
            {
                return true;
            }

            if (m_cursorX < 0 || m_cursorY < 0)
            {
                return false;
            }

            if (m_options.wrapAtColumnLimit && m_cursorX >= m_options.defaultColumns)
            {
                m_cursorX = 0;
                ++m_cursorY;
            }

            if (!m_grid.setCell(m_cursorX, m_cursorY, glyph, m_currentStyle, m_warnings))
            {
                return false;
            }

            m_cursorX += (measuredWidth == CellWidth::Two) ? 2 : 1;
            return true;
        }

        AnsiLoader::LoadResult fail(const std::string& message)
        {
            AnsiLoader::LoadResult result;
            result.success = false;
            result.detectedFileType = AnsiLoader::FileType::Ans;
            result.errorMessage = message;
            result.warnings = m_warnings;
            return result;
        }

    private:
        AnsiLoader::LoadOptions m_options;
        GridBuilder m_grid;

        int m_cursorX = 0;
        int m_cursorY = 0;

        int m_savedCursorX = 0;
        int m_savedCursorY = 0;

        std::optional<Style> m_currentStyle;
        std::optional<Style> m_savedStyle;

        std::vector<AnsiLoader::LoadWarning> m_warnings;
    };
}

namespace AnsiLoader
{
    FileType detectFileType(const std::string& filePath)
    {
        const std::string extension = getFileExtensionLower(filePath);

        if (extension == ".ans")
        {
            return FileType::Ans;
        }

        return FileType::Unknown;
    }

    LoadResult loadFromFile(const std::string& filePath)
    {
        return loadFromFile(filePath, LoadOptions{});
    }

    LoadResult loadFromFile(const std::string& filePath, const LoadOptions& options)
    {
        std::string bytes;
        if (!readAllBytes(filePath, bytes))
        {
            LoadResult result;
            result.success = false;
            result.detectedFileType = detectFileType(filePath);
            result.errorMessage = "Failed to open ANSI file.";
            return result;
        }

        LoadResult result = loadFromBytes(bytes, options);
        result.detectedFileType = detectFileType(filePath);
        return result;
    }

    LoadResult loadFromFile(const std::string& filePath, const Style& style)
    {
        LoadOptions options;
        options.baseStyle = style;
        return loadFromFile(filePath, options);
    }

    bool tryLoadFromFile(const std::string& filePath, TextObject& outObject)
    {
        const LoadResult result = loadFromFile(filePath);
        if (!result.success)
        {
            return false;
        }

        outObject = result.object;
        return true;
    }

    bool tryLoadFromFile(const std::string& filePath, TextObject& outObject, const LoadOptions& options)
    {
        const LoadResult result = loadFromFile(filePath, options);
        if (!result.success)
        {
            return false;
        }

        outObject = result.object;
        return true;
    }

    bool tryLoadFromFile(const std::string& filePath, TextObject& outObject, const Style& style)
    {
        const LoadResult result = loadFromFile(filePath, style);
        if (!result.success)
        {
            return false;
        }

        outObject = result.object;
        return true;
    }

    LoadResult loadFromBytes(std::string_view bytes, const LoadOptions& options)
    {
        Parser parser(options);
        return parser.parse(bytes);
    }

    const char* toString(FileType fileType)
    {
        switch (fileType)
        {
        case FileType::Auto: return "Auto";
        case FileType::Unknown: return "Unknown";
        case FileType::Ans: return "Ans";
        default: return "Unknown";
        }
    }

    const char* toString(LoadWarningCode warningCode)
    {
        switch (warningCode)
        {
        case LoadWarningCode::None: return "None";
        case LoadWarningCode::UnsupportedSequenceIgnored: return "UnsupportedSequenceIgnored";
        case LoadWarningCode::UnsupportedPrivateSequenceIgnored: return "UnsupportedPrivateSequenceIgnored";
        case LoadWarningCode::CursorClamped: return "CursorClamped";
        case LoadWarningCode::CanvasExpanded: return "CanvasExpanded";
        case LoadWarningCode::BareEscapeIgnored: return "BareEscapeIgnored";
        default: return "Unknown";
        }
    }
}