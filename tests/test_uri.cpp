// Unit tests for shawky/ShawkyUri.{h,cpp}. Portable: builds with g++/clang/MSVC.
// Arabic literals below are UTF-8 (MSVC: compile with /utf-8).

#include "../shawky/ShawkyUri.h"

#include <cstdio>
#include <string>

using namespace shawky;

static int g_fail = 0, g_pass = 0;

#define CHECK(cond)                                                              \
    do {                                                                         \
        if (cond) {                                                              \
            g_pass++;                                                            \
        } else {                                                                 \
            g_fail++;                                                            \
            std::printf("FAIL %s:%d: %s\n", __FILE__, __LINE__, #cond);          \
        }                                                                        \
    } while (0)

#define CHECK_EQ(a, b)                                                           \
    do {                                                                         \
        auto _a = (a);                                                           \
        auto _b = (b);                                                           \
        if (_a == _b) {                                                          \
            g_pass++;                                                            \
        } else {                                                                 \
            g_fail++;                                                            \
            std::printf("FAIL %s:%d: %s == %s\n", __FILE__, __LINE__, #a, #b);   \
        }                                                                        \
    } while (0)

static UriError Err(const std::string& uri) { return ParseUri(uri).error; }

static std::vector<DialRule> DefaultRules() {
    std::vector<DialRule> rules;
    std::string err;
    bool ok = ParseDialRules(kDefaultDialRules, rules, &err);
    CHECK(ok);
    return rules;
}

static DialResult Norm(const std::string& raw, const std::string& prefix = "") {
    return NormalizeNumber(raw, DefaultRules(), prefix);
}

static void TestUriValid() {
    // The exact form our gateway opens (Arabic percent-encoded).
    const char* uri =
        "shawkycall://call?phone=+201001234567&name=%D8%B2%D9%8A%D8%A7%D8%AF&salesman=sales01"
        "&project=%D8%A7%D9%84%D8%A5%D8%B3%D9%83%D9%86%D8%AF%D8%B1%D9%8A%D8%A9"
        "&site_state=%D8%B7%D9%88%D8%A8%20%D8%A7%D8%AD%D9%85%D8%B1&area=60&status=Fresh&lead_id=";
    UriResult r = ParseUri(uri);
    CHECK(r.ok());
    CHECK_EQ(r.action, std::string("call"));
    CHECK_EQ(r.phone, std::string("+201001234567"));
    CHECK_EQ(r.Get("name"), std::string("زياد"));
    CHECK_EQ(r.Get("project"), std::string("الإسكندرية"));
    CHECK_EQ(r.Get("site_state"), std::string("طوب احمر"));
    CHECK_EQ(r.Get("salesman"), std::string("sales01"));
    CHECK_EQ(r.Get("lead_id"), std::string(""));
    CHECK_EQ(r.Get("unknown"), std::string(""));

    // Raw (unencoded) UTF-8 Arabic, as some browsers may pass it.
    r = ParseUri("shawkycall://call?phone=01001234567&name=زياد");
    CHECK(r.ok());
    CHECK_EQ(r.Get("name"), std::string("زياد"));

    // Quoted, as Windows passes "%1".
    CHECK(ParseUri("\"shawkycall://call?phone=0100\"").ok());
    // Case-insensitive scheme / action / key, trailing slash, fragment, %2B.
    r = ParseUri("ShawkyCall://CALL/?PHONE=%2B201001234567#x");
    CHECK(r.ok());
    CHECK_EQ(r.phone, std::string("+201001234567"));
    // Without "//".
    CHECK(ParseUri("shawkycall:call?phone=0100").ok());
    // Empty segments and params without '=' are tolerated.
    CHECK(ParseUri("shawkycall://call?&&phone=0100&flag&").ok());
    // Unknown extra params do not fail.
    CHECK(ParseUri("shawkycall://call?phone=0100&future_param=x&another=%D8%A7").ok());
}

static void TestUriInvalid() {
    CHECK_EQ(Err(""), UriError::Empty);
    CHECK_EQ(Err("   "), UriError::Empty);
    CHECK_EQ(Err("sip:0100"), UriError::BadScheme);
    CHECK_EQ(Err("shawkycal://call?phone=1"), UriError::BadScheme);
    CHECK_EQ(Err("http://call?phone=1"), UriError::BadScheme);
    CHECK_EQ(Err("shawkycall://exec?phone=0100"), UriError::UnknownAction);
    CHECK_EQ(Err("shawkycall://?phone=0100"), UriError::UnknownAction);
    CHECK_EQ(Err("shawkycall://hangup?phone=0100"), UriError::UnknownAction);
    CHECK_EQ(Err("shawkycall://call/../../x?phone=0100"), UriError::BadPath);
    CHECK_EQ(Err("shawkycall://call/extra?phone=0100"), UriError::BadPath);
    CHECK_EQ(Err("shawkycall://call"), UriError::MissingPhone);
    CHECK_EQ(Err("shawkycall://call?name=x"), UriError::MissingPhone);
    CHECK_EQ(Err("shawkycall://call?phone="), UriError::MissingPhone);
    CHECK_EQ(Err("shawkycall://call?phone=%20%20"), UriError::MissingPhone);
    CHECK_EQ(Err("shawkycall://call?phone=0100&phone=0999"), UriError::DuplicatePhone);
    CHECK_EQ(Err("shawkycall://call?phone=0100&PHONE=0999"), UriError::DuplicatePhone);
    // Bad percent encoding.
    CHECK_EQ(Err("shawkycall://call?phone=%ZZ"), UriError::BadEncoding);
    CHECK_EQ(Err("shawkycall://call?phone=0100%"), UriError::BadEncoding);
    CHECK_EQ(Err("shawkycall://call?phone=0100%4"), UriError::BadEncoding);
    // Invalid UTF-8: overlong '/', lone continuation, surrogate, truncated.
    CHECK_EQ(Err("shawkycall://call?phone=0100&name=%C0%AF"), UriError::BadUtf8);
    CHECK_EQ(Err("shawkycall://call?phone=0100&name=%80"), UriError::BadUtf8);
    CHECK_EQ(Err("shawkycall://call?phone=0100&name=%ED%A0%80"), UriError::BadUtf8);
    CHECK_EQ(Err("shawkycall://call?phone=0100&name=%D8"), UriError::BadUtf8);
    // Control characters / injection of new lines, NUL, C1.
    CHECK_EQ(Err("shawkycall://call?phone=0100%00"), UriError::ControlChar);
    CHECK_EQ(Err("shawkycall://call?phone=0100&name=a%0D%0Ab"), UriError::ControlChar);
    CHECK_EQ(Err("shawkycall://call?phone=0100&name=%C2%85"), UriError::ControlChar);
    CHECK_EQ(Err("shawkycall://call?phone=0100&name=%1B[31m"), UriError::ControlChar);
    // Length limits.
    CHECK_EQ(Err("shawkycall://call?phone=0100&name=" + std::string(300, 'a')), UriError::ParamTooLong);
    CHECK_EQ(Err("shawkycall://call?phone=0100&name=" + std::string(3000, 'a')), UriError::TooLong);
}

static void TestUriInjectionReachesNormalizer() {
    // These parse (the URI itself is well-formed), but the phone must be
    // rejected by the normalizer, so nothing is dialed.
    const char* attacks[] = {
        "shawkycall://call?phone=0100%22%20%26%20calc.exe",
        "shawkycall://call?phone=0100;calc.exe",
        "shawkycall://call?phone=0100|cmd",
        "shawkycall://call?phone=0100%26calc",
        "shawkycall://call?phone=sip:evil@attacker.example",
        "shawkycall://call?phone=0100@10.0.0.1",
        "shawkycall://call?phone=*97",
        "shawkycall://call?phone=0100%23",
        "shawkycall://call?phone=%25n%25n%25s",
        "shawkycall://call?phone=0100`whoami`",
        "shawkycall://call?phone=$(reboot)",
        "shawkycall://call?phone=..%2F..%2Fwindows",
        "shawkycall://call?phone=+966501234567",
    };
    for (const char* a : attacks) {
        UriResult r = ParseUri(a);
        bool blocked = !r.ok() || !Norm(r.phone).ok();
        if (!blocked) std::printf("NOT BLOCKED: %s\n", a);
        CHECK(blocked);
    }
}

static void TestDialRules() {
    std::vector<DialRule> rules;
    std::string err;
    CHECK(ParseDialRules(kDefaultDialRules, rules, &err));
    CHECK_EQ(rules.size(), (size_t)3);
    CHECK_EQ(rules[0].from, std::string("+20"));
    CHECK_EQ(rules[2].minDigits, (size_t)11);
    CHECK(ParseDialRules("", rules, &err));
    CHECK_EQ(rules.size(), (size_t)0);
    CHECK(ParseDialRules(" +20 > 0 ; ", rules, &err));
    CHECK_EQ(rules.size(), (size_t)1);
    CHECK(ParseDialRules("+>00", rules, &err) == false);
    CHECK(ParseDialRules("20", rules, &err) == false);
    CHECK(ParseDialRules("20>x", rules, &err) == false);
    CHECK(ParseDialRules("2a>0", rules, &err) == false);
    CHECK(ParseDialRules("20>0:abc", rules, &err) == false);
    CHECK(ParseDialRules("20>0:", rules, &err) == false);
    CHECK(ParseDialRules("20>", rules, &err));  // strip prefix entirely: allowed
}

static void TestNormalize() {
    CHECK_EQ(Norm("+201001234567").number, std::string("01001234567"));
    CHECK_EQ(Norm("00201001234567").number, std::string("01001234567"));
    CHECK_EQ(Norm("201001234567").number, std::string("01001234567"));
    CHECK_EQ(Norm("01001234567").number, std::string("01001234567"));
    CHECK_EQ(Norm("+20 100 123 4567").number, std::string("01001234567"));
    CHECK_EQ(Norm("+20-100-123-4567").number, std::string("01001234567"));
    CHECK_EQ(Norm("(010) 0123.4567").number, std::string("01001234567"));
    // Cairo landline in international form: 20 + 2 + 8 digits = 11 digits.
    CHECK_EQ(Norm("+20223456789").number, std::string("0223456789"));
    CHECK_EQ(Norm("20223456789").number, std::string("0223456789"));
    // Short internal extensions are never rewritten by the "20" rule.
    CHECK_EQ(Norm("201").number, std::string("201"));
    CHECK_EQ(Norm("2001").number, std::string("2001"));
    // Arabic-Indic and Extended Arabic-Indic digits.
    CHECK_EQ(Norm("٠١٠٠١٢٣٤٥٦٧").number, std::string("01001234567"));
    CHECK_EQ(Norm("+٢٠١٠٠١٢٣٤٥٦٧").number, std::string("01001234567"));
    CHECK_EQ(Norm("۰۱۰۰۱۲۳۴۵۶۷").number, std::string("01001234567"));
    // Bidi marks and NBSP copied from RTL text.
    CHECK_EQ(Norm("\xE2\x80\x8E+20\xC2\xA0" "1001234567\xE2\x80\x8F").number, std::string("01001234567"));
    // Dial prefix.
    CHECK_EQ(Norm("+201001234567", "9").number, std::string("901001234567"));
    CHECK_EQ(Norm("+201001234567", "9x").error, DialError::BadPrefix);
    // Errors.
    CHECK_EQ(Norm("").error, DialError::Empty);
    CHECK_EQ(Norm("   ").error, DialError::Empty);
    CHECK_EQ(Norm("abc").error, DialError::InvalidChars);
    CHECK_EQ(Norm("+966501234567").error, DialError::InvalidChars);  // no rule for +966
    CHECK_EQ(Norm("0100*").error, DialError::InvalidChars);
    CHECK_EQ(Norm("0100#").error, DialError::InvalidChars);
    CHECK_EQ(Norm("1").error, DialError::TooShort);
    CHECK_EQ(Norm("0123456789012345").error, DialError::TooLong);    // 16 digits
    CHECK_EQ(Norm("012345678901234").number, std::string("012345678901234"));  // 15 ok
    CHECK_EQ(Norm("0123456789012", "999").error, DialError::TooLong);  // 16 with prefix
    // A rule can map '+' to 00 for other countries if configured.
    std::vector<DialRule> rules;
    std::string err;
    CHECK(ParseDialRules("+20>0;+>00", rules, &err) == false);  // '+' alone is not allowed
    CHECK(ParseDialRules("+20>0;+966>00966", rules, &err));
    CHECK_EQ(NormalizeNumber("+966501234567", rules, "").number, std::string("00966501234567"));
}

static void TestHelpers() {
    CHECK(IsShawkyUri("shawkycall://call?phone=1"));
    CHECK(IsShawkyUri("  \"SHAWKYCALL://x\"  "));
    CHECK(!IsShawkyUri("sip:123"));
    CHECK(!IsShawkyUri("shawky"));
    CHECK_EQ(MaskNumber("01001234567"), std::string("********567"));
    CHECK_EQ(MaskNumber("12"), std::string("**"));
    CHECK(IsValidUtf8("زياد"));
    CHECK(IsValidUtf8("\xF0\x9F\x93\x9E"));  // emoji
    CHECK(!IsValidUtf8("\xF4\x90\x80\x80"));  // > U+10FFFF
}

int main() {
    TestUriValid();
    TestUriInvalid();
    TestUriInjectionReachesNormalizer();
    TestDialRules();
    TestNormalize();
    TestHelpers();
    std::printf("%d passed, %d failed\n", g_pass, g_fail);
    return g_fail ? 1 : 0;
}
