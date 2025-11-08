#include "Logger.h"
#include <fstream>
#include <chrono>
#include <ctime>

Logger::Logger() {
    AutoScroll = false;
    ClearLog();
    memset(CmdBuf, 0, sizeof(CmdBuf));
}

Logger::~Logger() {

}

void Logger::ClearLog() {
    Buf.clear();
    LineOffsets.clear();
    LineOffsets.push_back(LineData(0, ImVec4(1.0f,1.0f,1.0f,1.0f)));
    counter = 0;
}

const char* Logger::GetSysTime() {
    std::chrono::system_clock::time_point now = std::chrono::system_clock::now();
    std::time_t now_c = std::chrono::system_clock::to_time_t(now);
    std::tm* local_tm = std::localtime(&now_c);
    char output[50];

    std::strftime(output, 50, "%H:%M:%S", local_tm);
    return output;
}

void Logger::Draw() {
    // ImGui::DockSpaceOverViewport();
    // ImGui::ShowDemoWindow();

    // begin() and end() are for windows
    ImGui::Begin("Game Log");

    bool cleared = ImGui::Button("Clear");
    bool info = ImGui::Button("Test Info"); ImGui::SameLine();
    bool warn = ImGui::Button("Test Warn"); ImGui::SameLine();
    bool error = ImGui::Button("Test Error"); ImGui::SameLine();
    Filter.Draw("Filter", -100.0f);

    bool saveToFile = ImGui::Button("Save To File"); ImGui::SameLine();
    ImGui::Separator();

    if (info) {
        LogInfo("This is a test info message");
    }
    if (warn) {
        LogWarn("This is a test warning message");
    }
    if (error) {
        LogError("This is a test error message");
    }
    if (ImGui::BeginChild("Scroll", ImVec2(0, 0), ImGuiChildFlags_None, ImGuiWindowFlags_HorizontalScrollbar))
    {
        if (cleared) {
            ClearLog();
        }

        ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(0, 0));
        const char* buf = Buf.begin();
        const char* buf_end = Buf.end();
        if (Filter.IsActive())
        {
            // In this example we don't use the clipper when Filter is enabled.
            // This is because we don't have random access to the result of our filter.
            // A real application processing logs with ten of thousands of entries may want to store the result of
            // search/filter.. especially if the filtering function is not trivial (e.g. reg-exp).
            for (int line_no = 0; line_no < LineOffsets.Size; line_no++)
            {
                const char* line_start = buf + LineOffsets[line_no].offset;
                const char* line_end = (line_no + 1 < LineOffsets.Size) ? (buf + LineOffsets[line_no + 1].offset - 1) : buf_end;
                if (Filter.PassFilter(line_start, line_end)) {
                    int a = (line_no + 1 < LineOffsets.Size) ? line_no + 1 : line_no;
                    ImGui::PushStyleColor(ImGuiCol_Text, LineOffsets[a].col);
                    ImGui::TextUnformatted(line_start, line_end);
                    ImGui::PopStyleColor();
                }
            }
        }
        else
        {
            // The simplest and easy way to display the entire buffer:
            //   ImGui::TextUnformatted(buf_begin, buf_end);
            // And it'll just work. TextUnformatted() has specialization for large blob of text and will fast-forward
            // to skip non-visible lines. Here we instead demonstrate using the clipper to only process lines that are
            // within the visible area.
            // If you have tens of thousands of items and their processing cost is non-negligible, coarse clipping them
            // on your side is recommended. Using ImGuiListClipper requires
            // - A) random access into your data
            // - B) items all being the  same height,
            // both of which we can handle since we have an array pointing to the beginning of each line of text.
            // When using the filter (in the block of code above) we don't have random access into the data to display
            // anymore, which is why we don't use the clipper. Storing or skimming through the search result would make
            // it possible (and would be recommended if you want to search through tens of thousands of entries).
            ImGuiListClipper clipper;
            clipper.Begin(LineOffsets.Size);
            while (clipper.Step())
            {
                for (int line_no = clipper.DisplayStart; line_no < clipper.DisplayEnd; line_no++)
                {
                    const char* line_start = buf + LineOffsets[line_no].offset;
                    const char* line_end = (line_no + 1 < LineOffsets.Size) ? (buf + LineOffsets[line_no + 1].offset - 1) : buf_end;
                    int a = (line_no + 1 < LineOffsets.Size) ? line_no + 1 : line_no;
                    ImGui::PushStyleColor(ImGuiCol_Text, LineOffsets[a].col);
                    ImGui::TextUnformatted(line_start, line_end);
                    ImGui::PopStyleColor();
                }
            }
            clipper.End();
        }
        ImGui::PopStyleVar();

        // Keep up at the bottom of the scroll region if we were already at the bottom at the beginning of the frame.
        // Using a scrollbar or mouse-wheel will take away from the bottom edge.
        if (AutoScroll && ImGui::GetScrollY() >= ImGui::GetScrollMaxY())
            ImGui::SetScrollHereY(1.0f);
    }
    ImGui::EndChild();

    ImGui::Separator();
    ImGuiInputTextFlags input_text_flags = ImGuiInputTextFlags_EnterReturnsTrue | ImGuiInputTextFlags_EscapeClearsAll;
    if (ImGui::InputText("commands", CmdBuf, IM_ARRAYSIZE(CmdBuf), input_text_flags)) {
        char* s = CmdBuf;
        if (s[0]) {
            LogInfo(s);
            strcpy(s, "");
        }
    }
    ImGui::SetItemDefaultFocus();

    if (saveToFile) {
        OutputToFile();
    }
    ImGui::End();
}



void IM_FMTARGS(3) Logger::AddLog(ImVec4 c, const char* fmt, ...) {
    {
        int old_size = Buf.size();
        va_list args;
        va_start(args, fmt);
        Buf.appendfv(fmt, args);
        va_end(args);
        for (int new_size = Buf.size(); old_size < new_size; old_size++)
            if (Buf[old_size] == '\n')
                LineOffsets.push_back(LineData(old_size + 1, c));

        counter++;
    }
}

// interface
Logger& Logger::GetInstance() {
    static Logger logger;
    return logger;
}

void Logger::LogInfo(const char* text) {
    AddLog(ImVec4(1.0f,1.0f,1.0f,1.0f), "[%s] [INFO] %s\n", GetSysTime(), text, counter);
}

void Logger::LogWarn(const char* text) {
    static float t = 0.0f; if (ImGui::GetTime() - t > 0.02f) { t = ImGui::GetTime(); }
    AddLog(ImVec4(1.0f,1.0f,0.0f,1.0f), "[%s] [WARN] %s\n", GetSysTime(), text, counter);
}

void Logger::LogError(const char* text) {
    static float t = 0.0f; if (ImGui::GetTime() - t > 0.02f) { t = ImGui::GetTime(); }
    AddLog(ImVec4(1.0f,0.0f,0.0f,1.0f), "[%s] [ERROR] %s\n", GetSysTime(), text, counter);
}

void Logger::LogGameEvent(const char* text, const std::string context) {
    std::string st = "[GAME] ";
    st.append(text);

    if (context == "Info") {
        LogInfo(st.c_str());
    }
    else if (context == "Warn")
    {
        LogWarn(st.c_str());
    }
    else if (context == "Error") 
    {
        LogError(st.c_str());
    }
}

void Logger::OutputToFile() {
    std::ofstream fs;
    fs.open("latest.txt");

    const char* buf = Buf.begin();
    const char* buf_end = Buf.end();

    for (int line_no = 0; line_no < LineOffsets.Size; line_no++) {
        const char* line_start = buf + LineOffsets[line_no].offset;
        const char* line_end = (line_no + 1 < LineOffsets.Size) ? (buf + LineOffsets[line_no + 1].offset - 1) : buf_end;

        fs.write(line_start, line_end - line_start);
        fs.write("\n", size_t(1));
    }

    fs.close();
}