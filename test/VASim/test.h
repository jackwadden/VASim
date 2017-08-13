//

void pass(std::string testname) {
    printf("%s PASSED\n", testname.c_str());
}

void pass(std::string testname, std::string message) {
    printf("%s:%s PASSED\n", testname.c_str(), message.c_str());
    exit(1);
}

void fail(std::string testname) {
    printf("%s FAILED\n", testname.c_str());
    exit(1);
}

void fail(std::string testname, std::string message) {
    printf("%s:%s FAILED\n", testname.c_str(), message.c_str());
    exit(1);
}

void assert(bool condition, std::string testname) {

    if(!condition)
        fail(testname);
}

void assert(bool condition, std::string testname, std::string message) {

    if(!condition)
        fail(testname, message);
}


