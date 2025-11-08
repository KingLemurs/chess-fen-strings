#include "../imgui/imgui.h"
#include <string>

struct LineData {
    int offset;
    ImVec4 col;

    LineData(int off, ImVec4 c) : offset(off), col(c) {}
};

// TODO write to file 

class Logger {
    private:
        int counter = 0;
        char CmdBuf[256];
        ImGuiTextBuffer     Buf;
        ImGuiTextFilter     Filter;
        // TODO CHANGE TO VEC OF LINEDATA
        ImVector<LineData>       LineOffsets; // Index to lines offset. We maintain this with AddLog() calls.
        bool                AutoScroll;  // Keep scrolling if already at the bottom.

        void ClearLog();
        void IM_FMTARGS(3) AddLog(ImVec4 c, const char* fmt, ...);
        const char* GetSysTime();
    public:
        Logger();
        ~Logger();

        static Logger& GetInstance();
        void LogInfo(const char* text);
        void LogWarn(const char* text);
        void LogError(const char* text);
        void OutputToFile();
        void LogGameEvent(const char* text, const std::string context = "Info");
        void Draw();
};