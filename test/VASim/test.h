//

void pass(std::string testname) {
    printf("%s PASSED\n", testname.c_str());
}

void fail(std::string testname) {
    printf("%s FAILED\n", testname.c_str());
    exit(1);
}


