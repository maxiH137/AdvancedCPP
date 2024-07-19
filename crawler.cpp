#include <iostream>
#include <string>
#include <list>
#include <queue>
#include <set>
#include <unordered_set>
#include <vector>
#include <sys/times.h>
#include <curl/curl.h>
#include <htmlcxx/html/ParserDom.h>
#include <boost/algorithm/string.hpp>
#include <chrono>

using namespace std;
using namespace htmlcxx;
using namespace boost::algorithm;

const size_t write_to_string(char *ptr, size_t size, size_t count, void *stream) {
  ((string*)stream)->append(ptr, 0, size*count);
  return size*count;
}

class Page {
    public:
      string url;
      string body;
     
      Page(string url, string body) : url(url), body(body){}

      // Rule of Three
      ~Page();
      Page(const Page& that); 
      Page & operator=(Page& that) = default;

      unordered_set<string> getUniqWords();
};

Page::~Page() {};

Page::Page(const Page& that) {
    this->url = that.url;
    this->body = that.body;
}

unordered_set<string> Page::getUniqWords() {
    unordered_set<string> uniqueWords;
    split(uniqueWords, this->body, is_space());
    return uniqueWords;
}


string getPageContent(const string url) {
  string content = "";

  CURL *curl = curl_easy_init();
  CURLcode res;

  if (curl) {
      curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
      curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_to_string);
      curl_easy_setopt(curl, CURLOPT_WRITEDATA, &content);
      res = curl_easy_perform(curl);

      curl_easy_cleanup(curl);
  }
  return content;
}

set<string> getLinks(const string html) {
  set<string> links;

  HTML::ParserDom parser;
  tree<HTML::Node> dom = parser.parseTree(html);

  for (auto it = dom.begin(); it != dom.end(); ++it) {
    if (it->tagName() == "a") {
      it->parseAttributes();
      links.insert(it->attribute("href").second);
    }
  }

  return links;
}

vector<Page> crawl(const string startUrl, const int depth) {
  vector<Page> pages;
  pages.reserve(depth);

  queue<string> unvisited;
  unvisited.push(startUrl);
 
  while (!unvisited.empty() && pages.size() < depth) {
     const string link = unvisited.front();
     unvisited.pop();

     const string html = getPageContent(link);
     set<string> links = getLinks(html);

     for (auto it = links.begin(); it != links.end(); ++it) {
         unvisited.push(*it);
     }
    
     Page page(link, html);
     pages.push_back(page);
  }
  return pages;
}

int main() {
  auto start = chrono::high_resolution_clock::now();
  const int n = 10; // depth
  const string url = "https://github.com/kristofvl/AdvancedCPP";

  auto start_crawl = chrono::high_resolution_clock::now();
  vector<Page> pages = crawl(url, n);
  auto end_crawl = chrono::high_resolution_clock::now();

  unordered_set<string> totalUniqWords;
  auto start_for = chrono::high_resolution_clock::now();

  for (auto it = pages.begin(); it != pages.end(); ++it) {
    unordered_set<string> uniqueWordsPerPage = it->getUniqWords();
    cout << it->url << ", Unique words: " << uniqueWordsPerPage.size() << endl;

    for (auto j = uniqueWordsPerPage.begin(); j != uniqueWordsPerPage.end(); ++j) {
          totalUniqWords.insert(*j);
    }
  }
  cout << "Total words: " << totalUniqWords.size() << endl;
  auto end= chrono::high_resolution_clock::now();


  auto duration_crawl = chrono::duration_cast<chrono::microseconds>(end_crawl - start_crawl).count();
  auto duration_for = chrono::duration_cast<chrono::microseconds>(end - start_for).count();
  auto duration_all = chrono::duration_cast<chrono::microseconds>(end - start).count();
  cout << "Time Crawl: " << duration_crawl << " microseconds.\n";
  cout << "Time For: " << duration_for << " microseconds.\n";
  cout << "Time All: " << duration_all << " microseconds.\n";

  return 0;
}
