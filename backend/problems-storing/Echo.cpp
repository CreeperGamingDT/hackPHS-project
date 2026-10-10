#include "util.cpp"
#include <string>
#include <sstream>
#include <cstdio>
#include <fstream>
#include <vector>
#include <set>
#include <algorithm>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <poll.h>
#include "Problems.cpp"

const int PORT = 5000;
const string PROBLEMS_FILE = "problems.txt"; // one saved problem per line

// Escape a string so it's safe to put inside JSON quotes
string jsonEscape(const string& s){
    string out;
    for (unsigned char c : s) {
        switch (c) {
            case '"':  out += "\\\""; break;
            case '\\': out += "\\\\"; break;  
            case '\n': out += "\\n";  break;
            case '\r': out += "\\r";  break;
            case '\t': out += "\\t";  break;
            case '\b': out += "\\b";  break;
            case '\f': out += "\\f";  break;
            default:
                if (c < 0x20) {
                    char hex[7];
                    snprintf(hex, sizeof(hex), "\\u%04x", c);
                    out += hex;
                } else {
                    out += c;
                }
        }
    }
    return out;
}

void sendResponse(int client, const string& status, const string& body){
    string response =
        "HTTP/1.1 " + status + "\r\n"
        "Content-Type: application/json\r\n"
        "Content-Length: " + to_string(body.size()) + "\r\n"
        "Connection: close\r\n"
        "\r\n" + body;
    write(client, response.c_str(), response.size());
}

// Add a Unicode code point to a string as UTF-8
void appendUtf8(string& out, unsigned int cp){
    if (cp < 0x80) out += (char)cp;
    else if (cp < 0x800) { out += (char)(0xC0 | (cp >> 6)); out += (char)(0x80 | (cp & 0x3F)); }
    else if (cp < 0x10000) { out += (char)(0xE0 | (cp >> 12)); out += (char)(0x80 | ((cp >> 6) & 0x3F)); out += (char)(0x80 | (cp & 0x3F)); }
    else { out += (char)(0xF0 | (cp >> 18)); out += (char)(0x80 | ((cp >> 12) & 0x3F)); out += (char)(0x80 | ((cp >> 6) & 0x3F)); out += (char)(0x80 | (cp & 0x3F)); }
}

// Find "key": "value" in a JSON body and put the decoded value in result.
// Returns false if the key is missing or its value isn't a string.
bool readJsonString(const string& json, const string& key, string& result){
    string quotedKey = "\"" + key + "\"";
    size_t pos = 0;
    while ((pos = json.find(quotedKey, pos)) != string::npos) {
        size_t i = pos + quotedKey.size();
        pos = i;
        while (i < json.size() && isspace((unsigned char)json[i])) i++;
        if (i >= json.size() || json[i] != ':') continue;
        i++;
        while (i < json.size() && isspace((unsigned char)json[i])) i++;
        if (i >= json.size() || json[i] != '"') return false;
        i++;

        result.clear();
        while (i < json.size() && json[i] != '"') {
            if (json[i] != '\\') { result += json[i++]; continue; }
            if (++i >= json.size()) return false;
            char e = json[i++];
            switch (e) {
                case 'n': result += '\n'; break;
                case 't': result += '\t'; break;
                case 'r': result += '\r'; break;
                case 'b': result += '\b'; break;
                case 'f': result += '\f'; break;
                case 'u': {
                    if (i + 4 > json.size()) return false;
                    unsigned int cp = stoul(json.substr(i, 4), nullptr, 16);
                    i += 4;
                    // Characters like emoji come as two \u codes (a "surrogate pair")
                    if (cp >= 0xD800 && cp <= 0xDBFF && i + 6 <= json.size() && json[i] == '\\' && json[i + 1] == 'u') {
                        unsigned int low = stoul(json.substr(i + 2, 4), nullptr, 16);
                        cp = 0x10000 + ((cp - 0xD800) << 10) + (low - 0xDC00);
                        i += 6;
                    }
                    appendUtf8(result, cp);
                    break;
                }
                default: result += e; // covers \" \\ and \/
            }
        }
        return i < json.size();
    }
    return false;
}

bool readJsonInteger(const string& json, const string& key, long long& result){
    string quotedKey = "\"" + key + "\"";
    size_t pos = json.find(quotedKey);
    if (pos == string::npos) return false;

    size_t i = pos + quotedKey.size();
    while (i < json.size() && isspace((unsigned char)json[i])) i++;
    if (i >= json.size() || json[i] != ':') return false;
    i++;
    while (i < json.size() && isspace((unsigned char)json[i])) i++;
    if (i >= json.size()) return false;

    size_t numberStart = i;
    if (json[i] == '-') i++;
    size_t digitsStart = i;
    while (i < json.size() && isdigit((unsigned char)json[i])) i++;
    if (i == digitsStart) return false;

    try {
        result = stoll(json.substr(numberStart, i - numberStart));
    } catch (const invalid_argument&) {
        return false;
    } catch (const out_of_range&) {
        return false;
    }
    return true;
}

vector<string> loadProblems(vector<long long>& userIds){
    vector<string> problems;
    userIds.clear();
    ifstream file(PROBLEMS_FILE);
    string line;
    while (getline(file, line)) {
        if (line.empty()) continue;

        // Only comma-separated records with a user ID are valid.
        size_t separator = line.find(',');
        if (separator == string::npos) continue;

        try {
            size_t parsed = 0;
            long long userId = stoll(line.substr(0, separator), &parsed);
            if (parsed != separator) continue;
            problems.push_back(line.substr(separator + 1));
            userIds.push_back(userId);
        } catch (const invalid_argument&) {
            continue;
        } catch (const out_of_range&) {
            continue;
        }
    }
    return problems;
}

// Remember a new problem (in memory and in the file) unless it's blank or already saved
void saveProblem(string problem, long long userId, vector<string>& problems, vector<long long>& userIds){
    for (char& c : problem) if (c == '\n' || c == '\r') c = ' '; // keep it on one line
    size_t start = problem.find_first_not_of(" \t");
    if (start == string::npos) return;
    problem = problem.substr(start, problem.find_last_not_of(" \t") - start + 1);

    string lowerNew = problem;
    for (char& c : lowerNew) c = tolower(c);
    for (const string& p : problems) {
        string lowerOld = p;
        for (char& c : lowerOld) c = tolower(c);
        if (lowerOld == lowerNew) return;
    }

    problems.push_back(problem);
    userIds.push_back(userId);
    ofstream(PROBLEMS_FILE, ios::app) << userId << "," << problem << "\n";
}

int main(){

    // Listen on both localhost addresses: 127.0.0.1 (IPv4) and ::1 (IPv6)
    int server4 = socket(AF_INET, SOCK_STREAM, 0);
    int server6 = socket(AF_INET6, SOCK_STREAM, 0);
    if (server4 < 0 || server6 < 0) { perror("socket"); return 1; }

    int opt = 1;
    setsockopt(server4, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
    setsockopt(server6, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
    setsockopt(server6, IPPROTO_IPV6, IPV6_V6ONLY, &opt, sizeof(opt));

    sockaddr_in addr4{};
    addr4.sin_family = AF_INET;
    addr4.sin_addr.s_addr = inet_addr("127.0.0.1"); // only this computer can connect
    addr4.sin_port = htons(PORT);

    sockaddr_in6 addr6{};
    addr6.sin6_family = AF_INET6;
    addr6.sin6_addr = in6addr_loopback;
    addr6.sin6_port = htons(PORT);

    if (::bind(server4, (sockaddr*)&addr4, sizeof(addr4)) < 0) { perror("bind 127.0.0.1"); return 1; }
    if (::bind(server6, (sockaddr*)&addr6, sizeof(addr6)) < 0) { perror("bind ::1"); return 1; }
    if (listen(server4, 10) < 0 || listen(server6, 10) < 0) { perror("listen"); return 1; }

    vector<long long> userIds;
    vector<string> problems = loadProblems(userIds);

    cout << "Listening on http://localhost:" << PORT << endl;
    cout << "Loaded " << problems.size() << " saved problems from " << PROBLEMS_FILE << endl;

    pollfd servers[2] = {{server4, POLLIN, 0}, {server6, POLLIN, 0}};

    while (true) {
        // Wait until one of the two sockets has a new connection
        if (poll(servers, 2, -1) < 0) continue;
        int ready = (servers[0].revents & POLLIN) ? server4 : server6;

        int client = accept(ready, nullptr, nullptr);
        if (client < 0) continue;

        // Read until we have all the headers (they end with a blank line)
        string request;
        char buffer[4096];
        size_t headerEnd;
        while ((headerEnd = request.find("\r\n\r\n")) == string::npos) {
            ssize_t n = read(client, buffer, sizeof(buffer));
            if (n <= 0) break;
            request.append(buffer, n);
        }
        if (headerEnd == string::npos) { close(client); continue; }

        string headers = request.substr(0, headerEnd);
        string body = request.substr(headerEnd + 4);

        // The first line looks like "POST /similar_problems HTTP/1.1"
        string method, path;
        istringstream(headers) >> method >> path;
        path = path.substr(0, path.find('?'));

        // Find Content-Length (header names aren't case sensitive)
        size_t contentLength = 0;
        string lower = headers;
        for (char& c : lower) c = tolower(c);
        size_t pos = lower.find("\r\ncontent-length:");
        if (pos != string::npos) contentLength = stoul(headers.substr(pos + 17));

        // Keep reading until we have the whole body
        while (body.size() < contentLength) {
            ssize_t n = read(client, buffer, sizeof(buffer));
            if (n <= 0) break;
            body.append(buffer, n);
        }
        body = body.substr(0, contentLength);

        cout << method << " " << path << " (" << body.size() << " bytes)" << endl;

        if (method != "POST") {
            sendResponse(client, "405 Method Not Allowed", "{\"error\": \"Only POST is allowed\"}");
        } else if (path == "/similar_problems") {
            string problem;
            long long userId;
            if (!readJsonString(body, "problem", problem) || !readJsonInteger(body, "user_id", userId)) {
                sendResponse(client, "400 Bad Request", "{\"error\": \"Body must be JSON like {\\\"problem\\\": \\\"...\\\", \\\"user_id\\\": 1234}\"}");
            } else {
                set<string> traits = assignTraits(problem);
                cout << "  traits:";
                for (const string& t : traits) cout << " " << t;
                cout << endl;

                vector<string> similar = findSimilar(problem, problems);
                string json = "{\"similar_problems\": [";
                for (size_t i = 0; i < similar.size(); i++) {
                    if (i > 0) json += ", ";

                    size_t ownerIndex = 0;
                    while (ownerIndex < problems.size() && problems[ownerIndex] != similar[i]) ownerIndex++;
                    long long ownerId = ownerIndex < userIds.size() ? userIds[ownerIndex] : 0;
                    json += "{\"problem\":\"" + jsonEscape(similar[i]) + "\",\"user_id\":" + to_string(ownerId) + "}";
                }
                json += "]}";
                saveProblem(problem, userId, problems, userIds);
                sendResponse(client, "200 OK", json);
            }
        } else {
            sendResponse(client, "200 OK", "{\"request\": \"" + jsonEscape(body) + "\"}");
        }
        close(client);
    }

}
