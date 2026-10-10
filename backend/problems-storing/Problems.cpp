// Gives each problem a set of traits (like "school" or "anxiety") and finds
// other problems that share them. Included by Echo.cpp.
#pragma once
#include <string>
#include <vector>
#include <set>
#include <map>
#include <cmath>
#include <algorithm>
using namespace std;

// How alike two problems must be (0 = no traits shared, 1 = same traits) to count as similar
const double SIMILARITY_THRESHOLD = 0.5;
const size_t MAX_RESULTS = 10;

// Words that give a problem each trait.
//   "test*" matches any word starting with "test" (test, tests, testing)
//   "mom"   matches only that exact word
//   "SAT"   (has capitals) matches only when written in capitals, so "I sat down" doesn't count
const map<string, vector<string>> TRAIT_WORDS = {
    {"school",        {"school*", "class", "classes", "classmate*", "classroom*", "teacher*", "semester*",
                       "professor*", "lecture*", "principal", "detention", "campus"}},
    {"study",         {"study*", "studi*", "revis*", "cram*", "learn*", "memoriz*", "notes"}},
    {"math",          {"math*", "algebra*", "geometry", "calculus", "calc", "trig*", "statistic*",
                       "equation*", "fraction*"}},
    {"science",       {"science*", "scientific", "biology", "bio", "chemistry", "chem", "physics"}},
    {"english",       {"english", "essay*", "literature", "grammar", "spelling", "vocab*"}},
    {"history",       {"history", "historical", "geography", "civics"}},
    {"language",      {"spanish", "french", "german", "chinese", "japanese", "latin"}},
    {"test",          {"test*", "exam*", "quiz*", "midterm*", "finals", "SAT", "ACT", "PSAT", "AP"}},
    {"homework",      {"homework", "assignment*", "worksheet*", "project*"}},
    {"grades",        {"grade*", "GPA", "fail*", "flunk*", "score*", "report"}},
    {"college",       {"college*", "universit*", "admission*", "applica*", "scholarship*", "major", "dorm*"}},
    {"stress",        {"stress*", "overwhelm*", "pressure*", "burnout", "burned", "overwork*", "tense"}},
    {"anxiety",       {"anxi*", "nervous*", "scare*", "scary", "afraid", "fear*", "panic*", "worr*",
                       "dread*", "terrified"}},
    {"sadness",       {"sad", "sadness", "sadder", "depress*", "unhappy", "cry", "crying", "cried",
                       "hopeless*", "miserable", "upset", "grief", "griev*"}},
    {"anger",         {"angry", "anger*", "mad", "furious", "annoy*", "frustrat*", "rage", "hate"}},
    {"loneliness",    {"lonely", "loneliness", "alone", "isolat*", "excluded", "ignored"}},
    {"sleep",         {"sleep*", "slept", "insomnia", "tired*", "exhaust*", "nap", "naps", "awake"}},
    {"health",        {"sick*", "ill", "illness", "doctor*", "hospital*", "pain*", "headache*", "injur*",
                       "health*", "disease", "medic*"}},
    {"mental_health", {"mental", "therap*", "counsel*", "ADHD", "OCD", "PTSD"}},
    {"family",        {"family", "families", "parent*", "mom", "moms", "mum", "mother", "dad", "dads",
                       "father", "brother*", "sister*", "sibling*", "grandma", "grandpa", "grandparent*",
                       "divorce*", "home"}},
    {"friends",       {"friend*", "bff", "bestie", "squad"}},
    {"relationships", {"boyfriend*", "girlfriend*", "crush*", "dating", "date", "breakup", "relationship*",
                       "partner*", "ex"}},
    {"bullying",      {"bully", "bullie*", "bullied", "bullying", "teas*", "harass*"}},
    {"social_media",  {"instagram", "tiktok", "snapchat", "twitter", "social", "online", "followers"}},
    {"money",         {"money", "cash", "broke", "afford*", "debt*", "pay", "paying", "rent", "bill",
                       "bills", "expensive", "budget*", "loan*"}},
    {"work",          {"job", "jobs", "work", "working", "boss*", "coworker*", "shift*", "hired", "fired",
                       "interview*", "intern*", "employ*", "salary"}},
    {"sports",        {"sport*", "team*", "coach*", "practice*", "game*", "soccer", "football", "basketball",
                       "baseball", "tennis", "swim*", "track", "tryout*", "athlet*"}},
    {"future",        {"future", "career*", "plan", "plans", "planning", "goal*", "dream*", "graduat*"}},
    {"time",          {"procrastinat*", "deadline*", "busy", "schedule*", "time", "late", "behind"}},
    {"motivation",    {"motivat*", "lazy", "focus*", "distract*", "bored*", "boring", "concentrat*"}},
    {"self_esteem",   {"confiden*", "insecur*", "ugly", "worthless", "failure", "stupid", "dumb",
                       "embarrass*", "compar*"}},
    {"body_image",    {"weight", "fat", "skinny", "appearance", "body", "acne"}},
    {"tech",          {"code", "coding", "program*", "computer*", "laptop*", "bug*", "crash*", "css",
                       "html", "javascript", "python", "software", "app", "website"}},
};

// Traits that come with other traits, e.g. a math problem is also a school problem
const map<string, vector<string>> IMPLIED_TRAITS = {
    {"math",     {"school"}},
    {"science",  {"school"}},
    {"english",  {"school"}},
    {"history",  {"school"}},
    {"language", {"school"}},
    {"grades",   {"school"}},
    {"test",     {"school", "study"}},
    {"homework", {"school", "study"}},
};

// Split text into words, keeping each word's original capitalization
vector<string> splitWords(string text){
    // Treat curly apostrophes (’) like normal ones so "I’m" reads as "Im"
    size_t pos;
    while ((pos = text.find("\xE2\x80\x99")) != string::npos) text.replace(pos, 3, "'");

    vector<string> words;
    string word;
    for (size_t i = 0; i <= text.size(); i++) {
        unsigned char c = i < text.size() ? text[i] : ' ';
        if (isalnum(c) || c >= 0x80) word += c;
        else if (c == '\'') continue; // "don't" -> "dont"
        else {
            if (!word.empty()) words.push_back(word);
            word.clear();
        }
    }
    return words;
}

bool wordMatches(const string& word, const string& trigger){
    // Triggers with capitals must match exactly (an optional "s" is allowed: "SATs")
    if (any_of(trigger.begin(), trigger.end(), ::isupper)) {
        return word == trigger || word == trigger + "s";
    }
    string lower = word;
    for (char& c : lower) c = tolower(c);
    if (trigger.back() == '*') {
        return lower.compare(0, trigger.size() - 1, trigger, 0, trigger.size() - 1) == 0;
    }
    return lower == trigger;
}

// Look through a problem's words and give it every trait they point to
set<string> assignTraits(const string& problem){
    set<string> traits;
    for (const string& word : splitWords(problem)) {
        for (const auto& [trait, triggers] : TRAIT_WORDS) {
            for (const string& trigger : triggers) {
                if (wordMatches(word, trigger)) { traits.insert(trait); break; }
            }
        }
    }

    set<string> implied;
    for (const string& t : traits) {
        auto it = IMPLIED_TRAITS.find(t);
        if (it != IMPLIED_TRAITS.end()) implied.insert(it->second.begin(), it->second.end());
    }
    traits.insert(implied.begin(), implied.end());
    return traits;
}

// Score from 0 to 1 for how many traits two problems share
double similarity(const set<string>& a, const set<string>& b){
    if (a.empty() || b.empty()) return 0;
    size_t shared = 0;
    for (const string& t : a) shared += b.count(t);
    return shared / sqrt((double)a.size() * b.size());
}

// Saved problems that share enough traits with the new one, most similar first
vector<string> findSimilar(const string& problem, const vector<string>& problems){
    set<string> target = assignTraits(problem);
    vector<pair<double, string>> scored;
    for (const string& p : problems) {
        double score = similarity(target, assignTraits(p));
        if (score >= SIMILARITY_THRESHOLD) scored.push_back({score, p});
    }
    stable_sort(scored.begin(), scored.end(), [](const auto& x, const auto& y){ return x.first > y.first; });

    vector<string> result;
    for (size_t i = 0; i < scored.size() && i < MAX_RESULTS; i++) result.push_back(scored[i].second);
    return result;
}
