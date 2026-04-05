#include "Rendering/Objects/AnsiLoader.h"

#include <algorithm>
#include <cctype>
#include <cstdint>
#include <fstream>
#include <iterator>
#include <sstream>
#include <string>
#include <vector>

#include "Rendering/Objects/TextObjectBuilder.h"
#include "Rendering/Styles/Color.h"
#include "Utilities/Unicode/UnicodeConversion.h"
#include "Utilities/Unicode/UnicodeWidth.h"

namespace
{
    struct SauceRecord
    {
        bool found = false;
        bool truncated = false;
        std::size_t contentSize = 0;
        AnsiLoader::SauceMetadata metadata;
    };

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

    std::string trimRightAscii(const std::string& value)
    {
        std::size_t end = value.size();
        while (end > 0 && (value[end - 1] == ' ' || value[end - 1] == '\0'))
        {
            --end;
        }

        return value.substr(0, end);
    }

    std::uint16_t readLe16(std::string_view bytes, std::size_t offset, bool& ok)
    {
        if (offset + 1 >= bytes.size())
        {
            ok = false;
            return 0;
        }

        ok = true;
        return static_cast<std::uint16_t>(
            static_cast<unsigned char>(bytes[offset]) |
            (static_cast<unsigned char>(bytes[offset + 1]) << 8));
    }

    SauceRecord parseSauce(std::string_view bytes)
    {
        SauceRecord result;
        result.contentSize = bytes.size();

        if (bytes.size() < 128)
        {
            return result;
        }

        const std::size_t sauceOffset = bytes.size() - 128;
        const std::string_view sauce = bytes.substr(sauceOffset, 128);

        if (sauce.substr(0, 5) != "SAUCE")
        {
            return result;
        }

        result.found = true;
        result.contentSize = sauceOffset;

        result.metadata.present = true;
        result.metadata.title = trimRightAscii(std::string(sauce.substr(7, 35)));
        result.metadata.author = trimRightAscii(std::string(sauce.substr(42, 20)));
        result.metadata.group = trimRightAscii(std::string(sauce.substr(62, 20)));
        result.metadata.date = trimRightAscii(std::string(sauce.substr(82, 8)));
        result.metadata.dataType = static_cast<std::uint8_t>(sauce[94]);
        result.metadata.fileType = static_cast<std::uint8_t>(sauce[95]);

        bool ok = false;
        result.metadata.tInfo1 = readLe16(sauce, 96, ok);
        result.metadata.tInfo2 = readLe16(sauce, 98, ok);
        result.metadata.tInfo3 = readLe16(sauce, 100, ok);
        result.metadata.tInfo4 = readLe16(sauce, 102, ok);
        result.metadata.commentLineCount = static_cast<std::uint8_t>(sauce[104]);
        result.metadata.flags = static_cast<std::uint8_t>(sauce[105]);
        result.metadata.fontName = trimRightAscii(std::string(sauce.substr(106, 22)));

        const std::size_t commentBytes =
            (result.metadata.commentLineCount > 0)
            ? (5u + static_cast<std::size_t>(result.metadata.commentLineCount) * 64u)
            : 0u;

        if (commentBytes > 0)
        {
            if (result.contentSize < commentBytes)
            {
                result.truncated = true;
                return result;
            }

            const std::size_t commentBlockOffset = result.contentSize - commentBytes;
            if (bytes.substr(commentBlockOffset, 5) == "COMNT")
            {
                result.contentSize = commentBlockOffset;
            }
        }

        return result;
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

    Color::Basic ansiBasicToColor(int value)
    {
        switch (value)
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

    Color::Basic ansiBrightBasicToColor(int value)
    {
        switch (value)
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

    void addWarning(
        AnsiLoader::LoadResult& result,
        AnsiLoader::LoadWarningCode code,
        const std::string& message,
        std::size_t byteOffset,
        const AnsiLoader::SourcePosition& position)
    {
        AnsiLoader::LoadWarning warning;
        warning.code = code;
        warning.message = message;
        warning.byteOffset = byteOffset;
        warning.sourcePosition = position;
        result.warnings.push_back(warning);
    }

    class Parser
    {
    public:
        Parser(
            const AnsiLoader::LoadOptions& options,
            const SauceRecord& sauce)
            : m_options(options)
            , m_sauce(sauce)
        {
            m_width = options.defaultColumns;
            if (options.preferSauceWidth && sauce.metadata.present && sauce.metadata.tInfo1 > 0)
            {
                m_width = static_cast<int>(sauce.metadata.tInfo1);
            }

            if (m_width <= 0)
            {
                m_width = 80;
            }

            m_cursorStyle = options.baseStyle;
        }

        AnsiLoader::LoadResult parse(std::string_view bytes)
        {
            preScanDimensions(bytes);

            if (m_height <= 0)
            {
                m_height = 1;
            }

            TextObjectBuilder builder(m_width, m_height);

            for (std::size_t i = 0; i < bytes.size(); ++i)
            {
                const unsigned char byte = static_cast<unsigned char>(bytes[i]);

                if (byte == 0x1B)
                {
                    if (!consumeEscape(bytes, i))
                    {
                        return fail("Malformed or unsupported ANSI escape sequence.", i);
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
                        m_cursorX = 0;
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
                    m_cursorX = ((m_cursorX / 8) + 1) * 8;
                    clampCursor(i);
                    continue;
                }

                const char32_t glyph = decodeCp437Byte(byte);
                if (!writeGlyph(builder, glyph, i))
                {
                    return fail("ANSI content exceeded configured canvas bounds.", i, glyph);
                }
            }

            AnsiLoader::LoadResult result;
            result.object = builder.build();
            result.success = true;
            result.detectedFileType = AnsiLoader::FileType::Ans;
            result.resolvedWidth = result.object.getWidth();
            result.resolvedHeight = result.object.getHeight();
            result.sauce = m_sauce.metadata;

            if (m_sauce.metadata.present)
            {
                addWarning(
                    result,
                    AnsiLoader::LoadWarningCode::SauceMetadataPresent,
                    "SAUCE metadata was detected and parsed.",
                    0,
                    {});
            }

            if (m_options.preferSauceWidth && m_sauce.metadata.present && m_sauce.metadata.tInfo1 > 0)
            {
                addWarning(
                    result,
                    AnsiLoader::LoadWarningCode::SauceWidthOverrideApplied,
                    "SAUCE width metadata was applied to ANSI import.",
                    0,
                    {});
            }

            if (m_sauce.truncated)
            {
                addWarning(
                    result,
                    AnsiLoader::LoadWarningCode::TruncatedSauceIgnored,
                    "SAUCE comment metadata appeared truncated and was partially ignored.",
                    m_sauce.contentSize,
                    {});
            }

            result.warnings.insert(
                result.warnings.end(),
                m_warnings.begin(),
                m_warnings.end());

            return result;
        }

    private:
        void preScanDimensions(std::string_view bytes)
        {
            int x = 0;
            int y = 0;
            int maxX = 0;
            int maxY = 0;

            for (std::size_t i = 0; i < bytes.size(); ++i)
            {
                const unsigned char byte = static_cast<unsigned char>(bytes[i]);

                if (byte == 0x1B)
                {
                    if (i + 1 < bytes.size() && bytes[i + 1] == '[')
                    {
                        while (i + 1 < bytes.size())
                        {
                            ++i;
                            const unsigned char ch = static_cast<unsigned char>(bytes[i]);
                            if (ch >= 0x40 && ch <= 0x7E)
                            {
                                break;
                            }
                        }
                    }
                    continue;
                }

                if (byte == '\r')
                {
                    x = 0;
                    continue;
                }

                if (byte == '\n')
                {
                    x = 0;
                    ++y;
                    maxY = std::max(maxY, y);
                    continue;
                }

                if (byte == '\b')
                {
                    x = std::max(0, x - 1);
                    continue;
                }

                if (byte == '\t')
                {
                    x = ((x / 8) + 1) * 8;
                    maxX = std::max(maxX, x);
                    continue;
                }

                const CellWidth measuredWidth =
                    UnicodeWidth::measureCodePointWidth(decodeCp437Byte(byte));

                if (measuredWidth == CellWidth::Two)
                {
                    x += 2;
                }
                else if (measuredWidth != CellWidth::Zero)
                {
                    x += 1;
                }

                maxX = std::max(maxX, x);
            }

            if (m_options.expandCanvasOnDemand)
            {
                m_width = std::clamp(std::max(m_width, maxX), 1, m_options.maxColumns);
            }

            m_height = std::clamp(std::max(1, maxY + 1), 1, m_options.maxRows);
        }

        bool consumeEscape(std::string_view bytes, std::size_t& ioIndex)
        {
            if (ioIndex + 1 >= bytes.size())
            {
                AnsiLoader::LoadWarning warning;
                warning.code = AnsiLoader::LoadWarningCode::BareEscapeIgnored;
                warning.message = "Trailing ESC byte was ignored.";
                warning.byteOffset = ioIndex;
                warning.sourcePosition = { m_cursorX, m_cursorY };
                m_warnings.push_back(warning);

                return !m_options.strictUnsupportedCommands;
            }

            if (bytes[ioIndex + 1] != '[')
            {
                AnsiLoader::LoadWarning warning;
                warning.code = AnsiLoader::LoadWarningCode::UnsupportedSequenceIgnored;
                warning.message = "Non-CSI escape sequence was ignored.";
                warning.byteOffset = ioIndex;
                warning.sourcePosition = { m_cursorX, m_cursorY };
                m_warnings.push_back(warning);

                ++ioIndex;
                return !m_options.strictUnsupportedCommands;
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
                    return applyCsi(payload, finalByte, ioIndex);
                }

                payload.push_back(static_cast<char>(ch));
                ++cursor;
            }

            return false;
        }

        std::vector<int> parseParams(const std::string& body) const
        {
            if (body.empty())
            {
                return {};
            }

            std::vector<int> result;
            std::string token;

            for (char ch : body)
            {
                if (ch == ';')
                {
                    result.push_back(token.empty() ? 0 : std::stoi(token));
                    token.clear();
                    continue;
                }

                if (std::isdigit(static_cast<unsigned char>(ch)))
                {
                    token.push_back(ch);
                }
            }

            result.push_back(token.empty() ? 0 : std::stoi(token));
            return result;
        }

        int firstOr(const std::vector<int>& params, int fallback) const
        {
            if (params.empty())
            {
                return fallback;
            }

            return params[0] == 0 ? fallback : params[0];
        }

        int paramAt(const std::vector<int>& params, std::size_t index, int fallback) const
        {
            if (index >= params.size())
            {
                return fallback;
            }

            return params[index] == 0 ? fallback : params[index];
        }

        bool applyCsi(const std::string& payload, char finalByte, std::size_t byteOffset)
        {
            bool privateMode = false;
            std::string body = payload;

            if (!body.empty() && (body[0] == '?' || body[0] == '=' || body[0] == '>'))
            {
                privateMode = true;
                body.erase(body.begin());
            }

            if (privateMode)
            {
                AnsiLoader::LoadWarning warning;
                warning.code = AnsiLoader::LoadWarningCode::UnsupportedPrivateSequenceIgnored;
                warning.message = "Private CSI sequence was ignored.";
                warning.byteOffset = byteOffset;
                warning.sourcePosition = { m_cursorX, m_cursorY };
                m_warnings.push_back(warning);
                return !m_options.strictUnsupportedCommands;
            }

            const std::vector<int> params = parseParams(body);

            switch (finalByte)
            {
            case 'm':
                return applySgr(params, byteOffset);

            case 'A':
                m_cursorY -= firstOr(params, 1);
                clampCursor(byteOffset);
                return true;

            case 'B':
                m_cursorY += firstOr(params, 1);
                clampCursor(byteOffset);
                return true;

            case 'C':
                m_cursorX += firstOr(params, 1);
                clampCursor(byteOffset);
                return true;

            case 'D':
                m_cursorX -= firstOr(params, 1);
                clampCursor(byteOffset);
                return true;

            case 'H':
            case 'f':
                m_cursorY = std::max(0, firstOr(params, 1) - 1);
                m_cursorX = std::max(0, paramAt(params, 1, 1) - 1);
                clampCursor(byteOffset);
                return true;

            case 'G':
                m_cursorX = std::max(0, firstOr(params, 1) - 1);
                clampCursor(byteOffset);
                return true;

            case 's':
                m_savedX = m_cursorX;
                m_savedY = m_cursorY;
                m_savedStyle = m_cursorStyle;
                return true;

            case 'u':
                m_cursorX = m_savedX;
                m_cursorY = m_savedY;
                m_cursorStyle = m_savedStyle;
                clampCursor(byteOffset);
                return true;

            default:
            {
                AnsiLoader::LoadWarning warning;
                warning.code = AnsiLoader::LoadWarningCode::UnsupportedSequenceIgnored;
                warning.message = std::string("Unsupported CSI command ignored: ") + finalByte;
                warning.byteOffset = byteOffset;
                warning.sourcePosition = { m_cursorX, m_cursorY };
                m_warnings.push_back(warning);
                return !m_options.strictUnsupportedCommands;
            }
            }
        }

        void clampCursor(std::size_t byteOffset)
        {
            if (!m_options.clampCursorToBounds)
            {
                return;
            }

            const int oldX = m_cursorX;
            const int oldY = m_cursorY;

            m_cursorX = std::clamp(m_cursorX, 0, std::max(0, m_width - 1));
            m_cursorY = std::clamp(m_cursorY, 0, std::max(0, m_height - 1));

            if (m_cursorX != oldX || m_cursorY != oldY)
            {
                AnsiLoader::LoadWarning warning;
                warning.code = AnsiLoader::LoadWarningCode::CursorClamped;
                warning.message = "Cursor movement exceeded configured bounds and was clamped.";
                warning.byteOffset = byteOffset;
                warning.sourcePosition = { m_cursorX, m_cursorY };
                m_warnings.push_back(warning);
            }
        }

        Style effectiveStyle() const
        {
            return m_cursorStyle.value_or(Style{});
        }

        bool applyExtendedColor(
            const std::vector<int>& params,
            std::size_t& ioIndex,
            bool foreground)
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
                    m_cursorStyle = effectiveStyle().withForeground(
                        Color::FromIndexed(static_cast<std::uint8_t>(value)));
                }
                else
                {
                    m_cursorStyle = effectiveStyle().withBackground(
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
                    m_cursorStyle = effectiveStyle().withForeground(
                        Color::FromRgb(
                            static_cast<std::uint8_t>(r),
                            static_cast<std::uint8_t>(g),
                            static_cast<std::uint8_t>(b)));
                }
                else
                {
                    m_cursorStyle = effectiveStyle().withBackground(
                        Color::FromRgb(
                            static_cast<std::uint8_t>(r),
                            static_cast<std::uint8_t>(g),
                            static_cast<std::uint8_t>(b)));
                }

                return true;
            }

            return false;
        }

        bool applySgr(const std::vector<int>& params, std::size_t byteOffset)
        {
            if (params.empty())
            {
                m_cursorStyle = m_options.baseStyle;
                return true;
            }

            for (std::size_t i = 0; i < params.size(); ++i)
            {
                const int p = params[i];

                switch (p)
                {
                case 0:  m_cursorStyle = m_options.baseStyle; break;
                case 1:  m_cursorStyle = effectiveStyle().withBold(true); break;
                case 2:  m_cursorStyle = effectiveStyle().withDim(true); break;
                case 4:  m_cursorStyle = effectiveStyle().withUnderline(true); break;
                case 5:  m_cursorStyle = effectiveStyle().withSlowBlink(true); break;
                case 6:  m_cursorStyle = effectiveStyle().withFastBlink(true); break;
                case 7:  m_cursorStyle = effectiveStyle().withReverse(true); break;
                case 8:  m_cursorStyle = effectiveStyle().withInvisible(true); break;
                case 9:  m_cursorStyle = effectiveStyle().withStrike(true); break;

                case 22: m_cursorStyle = effectiveStyle().withoutBold().withoutDim(); break;
                case 24: m_cursorStyle = effectiveStyle().withoutUnderline(); break;
                case 25: m_cursorStyle = effectiveStyle().withoutSlowBlink().withoutFastBlink(); break;
                case 27: m_cursorStyle = effectiveStyle().withoutReverse(); break;
                case 28: m_cursorStyle = effectiveStyle().withoutInvisible(); break;
                case 29: m_cursorStyle = effectiveStyle().withoutStrike(); break;

                case 39: m_cursorStyle = effectiveStyle().withoutForeground(); break;
                case 49: m_cursorStyle = effectiveStyle().withoutBackground(); break;

                default:
                    if (p >= 30 && p <= 37)
                    {
                        m_cursorStyle = effectiveStyle().withForeground(
                            Color::FromBasic(ansiBasicToColor(p - 30)));
                    }
                    else if (p >= 40 && p <= 47)
                    {
                        m_cursorStyle = effectiveStyle().withBackground(
                            Color::FromBasic(ansiBasicToColor(p - 40)));
                    }
                    else if (p >= 90 && p <= 97)
                    {
                        m_cursorStyle = effectiveStyle().withForeground(
                            Color::FromBasic(ansiBrightBasicToColor(p - 90)));
                    }
                    else if (p >= 100 && p <= 107)
                    {
                        m_cursorStyle = effectiveStyle().withBackground(
                            Color::FromBasic(ansiBrightBasicToColor(p - 100)));
                    }
                    else if (p == 38 || p == 48)
                    {
                        const bool foreground = (p == 38);
                        if (!applyExtendedColor(params, i, foreground))
                        {
                            AnsiLoader::LoadWarning warning;
                            warning.code = AnsiLoader::LoadWarningCode::UnsupportedSgrIgnored;
                            warning.message = "Malformed extended SGR color sequence ignored.";
                            warning.byteOffset = byteOffset;
                            warning.sourcePosition = { m_cursorX, m_cursorY };
                            m_warnings.push_back(warning);

                            if (m_options.strictUnsupportedCommands)
                            {
                                return false;
                            }
                        }
                    }
                    else
                    {
                        AnsiLoader::LoadWarning warning;
                        warning.code = AnsiLoader::LoadWarningCode::UnsupportedSgrIgnored;
                        warning.message = "Unsupported SGR attribute ignored.";
                        warning.byteOffset = byteOffset;
                        warning.sourcePosition = { m_cursorX, m_cursorY };
                        m_warnings.push_back(warning);

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

        bool writeGlyph(TextObjectBuilder& builder, char32_t glyph, std::size_t byteOffset)
        {
            glyph = UnicodeConversion::sanitizeCodePoint(glyph);

            const CellWidth measuredWidth = UnicodeWidth::measureCodePointWidth(glyph);
            if (measuredWidth == CellWidth::Zero)
            {
                return true;
            }

            if (m_options.wrapAtDefaultColumn && m_cursorX >= m_width)
            {
                m_cursorX = 0;
                ++m_cursorY;
            }

            if (m_cursorX < 0 || m_cursorY < 0 || m_cursorY >= m_height)
            {
                return false;
            }

            const std::optional<Style> style = m_cursorStyle;

            bool ok = false;
            if (measuredWidth == CellWidth::Two)
            {
                ok = builder.setWideGlyph(m_cursorX, m_cursorY, glyph, style);
                m_cursorX += 2;
            }
            else
            {
                ok = builder.setGlyph(m_cursorX, m_cursorY, glyph, style);
                m_cursorX += 1;
            }

            if (!ok)
            {
                AnsiLoader::LoadWarning warning;
                warning.code = AnsiLoader::LoadWarningCode::CanvasExpanded;
                warning.message = "Parsed ANSI glyph reached beyond available builder bounds.";
                warning.byteOffset = byteOffset;
                warning.sourcePosition = { m_cursorX, m_cursorY };
                m_warnings.push_back(warning);
            }

            return ok;
        }

        AnsiLoader::LoadResult fail(
            const std::string& message,
            std::size_t byteOffset,
            char32_t codePoint = U'\0')
        {
            AnsiLoader::LoadResult result;
            result.success = false;
            result.detectedFileType = AnsiLoader::FileType::Ans;
            result.sauce = m_sauce.metadata;
            result.warnings = m_warnings;
            result.hasParseFailure = true;
            result.failingByteOffset = byteOffset;
            result.firstFailingCodePoint = codePoint;
            result.firstFailingPosition = { m_cursorX, m_cursorY };
            result.errorMessage = message;
            return result;
        }

    private:
        AnsiLoader::LoadOptions m_options;
        SauceRecord m_sauce;

        int m_width = 80;
        int m_height = 1;

        int m_cursorX = 0;
        int m_cursorY = 0;

        int m_savedX = 0;
        int m_savedY = 0;

        std::optional<Style> m_cursorStyle;
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
        const SauceRecord sauce = parseSauce(bytes);
        const std::string_view content = bytes.substr(0, sauce.contentSize);

        Parser parser(options, sauce);
        return parser.parse(content);
    }

    bool hasWarning(const LoadResult& result, LoadWarningCode code)
    {
        for (const LoadWarning& warning : result.warnings)
        {
            if (warning.code == code)
            {
                return true;
            }
        }

        return false;
    }

    const LoadWarning* getWarningByCode(const LoadResult& result, LoadWarningCode code)
    {
        for (const LoadWarning& warning : result.warnings)
        {
            if (warning.code == code)
            {
                return &warning;
            }
        }

        return nullptr;
    }

    std::string formatLoadError(const LoadResult& result)
    {
        std::ostringstream stream;
        stream << "ANSI load failed";
        if (!result.errorMessage.empty())
        {
            stream << ": " << result.errorMessage;
        }

        if (result.hasParseFailure)
        {
            stream << " [byte=" << result.failingByteOffset;

            if (result.firstFailingPosition.isValid())
            {
                stream << ", x=" << result.firstFailingPosition.x
                    << ", y=" << result.firstFailingPosition.y;
            }

            if (result.firstFailingCodePoint != U'\0')
            {
                stream << ", codepoint=U+"
                    << std::hex << std::uppercase
                    << static_cast<std::uint32_t>(result.firstFailingCodePoint)
                    << std::dec;
            }

            stream << "]";
        }

        return stream.str();
    }

    std::string formatLoadSuccess(const LoadResult& result)
    {
        std::ostringstream stream;
        stream << "ANSI load succeeded: "
            << result.resolvedWidth << "x" << result.resolvedHeight;

        if (result.sauce.present)
        {
            stream << ", SAUCE title=\"" << result.sauce.title << "\"";
        }

        if (!result.warnings.empty())
        {
            stream << ", warnings=" << result.warnings.size();
        }

        return stream.str();
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
        case LoadWarningCode::SauceMetadataPresent: return "SauceMetadataPresent";
        case LoadWarningCode::SauceWidthOverrideApplied: return "SauceWidthOverrideApplied";
        case LoadWarningCode::UnsupportedSequenceIgnored: return "UnsupportedSequenceIgnored";
        case LoadWarningCode::UnsupportedPrivateSequenceIgnored: return "UnsupportedPrivateSequenceIgnored";
        case LoadWarningCode::UnsupportedSgrIgnored: return "UnsupportedSgrIgnored";
        case LoadWarningCode::BareEscapeIgnored: return "BareEscapeIgnored";
        case LoadWarningCode::CursorClamped: return "CursorClamped";
        case LoadWarningCode::CanvasExpanded: return "CanvasExpanded";
        case LoadWarningCode::TruncatedSauceIgnored: return "TruncatedSauceIgnored";
        default: return "Unknown";
        }
    }
}