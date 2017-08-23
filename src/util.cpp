/**
 * @file
 */
#include "util.h"

/**
 * Writes ints in vector vec one per line to file with filename fn.
 */
void writeIntVectorToFile(std::vector<uint32_t> &vec, std::string fn) {

    std::ofstream out(fn);
    for(uint32_t val : vec)
        out << val << std::endl;
    out.close();
}

/**
 * Appends string str to file with filename fn.
 */
void appendStringToFile(std::string str, std::string fn) {

    std::ofstream out(fn, std::ios::out | std::ios::app);
    out << str;
    out.close();
}

/**
 * Writes string str to file with filename fn.
 */
void writeStringToFile(std::string str, std::string fn) {

    std::ofstream out(fn);
    out << str;
    out.close();
}

/**
 * Given a string, finds the first occurence of '.' from the right and returns everything after.
 */
std::string getFileExt(const std::string& s) {

   size_t i = s.rfind('.', s.length());
   if (i != std::string::npos) {
      return(s.substr(i+1, s.length() - i));
   }

   return("");
}

/**
 * Converts bitset to string representation of character set.
 */
std::string bitsetToCharset(std::bitset<256> column) {

    std::stringstream stream;
    stream << "[";

    size_t last_val;
    bool first = true;
    bool range = false;
    for(size_t i = 0; i < 256; i++){

        if(column.test(i)){
            
            // handle first occurence of a value
            if(first){
                last_val = i;
                first = false;
            }

            // if we're not in a range, check to see if we need to emit a dash
            if(!range) {
                // if this is a consecutive number, set range
                if(last_val == (i - 1)) {
                    range = true;
                    stream << "-";
                }else{
                    stream << "\\x" << std::setfill('0') << std::setw(2) << std::hex << i;
                }
            }else{

                // if we are in a range, a dash has already been set
                // check to see if we need to end the range
                if(last_val != (i - 1)) {
                    // if we're out of range
                    // print the last value
                    stream << "\\x" << std::setfill('0') << std::setw(2) << std::hex << last_val;
                    // print current value
                    stream << "\\x" << std::setfill('0') << std::setw(2) << std::hex << i;

                    // indicate we're no longer in a range
                    range = false;
                }

                // otherwise do nothing
            }

            last_val = i;
        }
    }

    // if we were in a range when we finished, make sure to emit the last value
    if(range){
        stream << "\\x" << std::setfill('0') << std::setw(2) << std::hex << last_val;
    }

    // emit final bracket
    stream << "]";

    return stream.str();
}

/**
 * Sets a range for a given bitset from start to end as value.
 */
void setRange(std::bitset<256> &column, int start, int end, int value) {

    for(;start <= end; start++){
        column.set(start, value);
    }
}

/**
 * Parses a string representation of a symbol/character set and sets each corresponding bit in column.
 */
void parseSymbolSet(std::bitset<256> &column, std::string symbol_set) {

    if(symbol_set.compare("*") == 0){
        column.set();
        return;
    }
    
    // KAA found that apcompile parses symbol-set="." to mean "^\x0a"
    // hard-coding this here
    if(symbol_set.compare(".") == 0) {
        column.set('\n',1);
        column.flip();
        return;
    }
        
    bool in_charset = false;
    bool escaped = false;
    bool inverting = false;
    bool range_set = false;
    int bracket_sem = 0;
    int brace_sem = 0;
    const unsigned int value = 1;
    unsigned char last_char = 0;
    unsigned char range_start = 0;

    // handle symbol sets that start and end with curly braces {###}
    if((symbol_set[0] == '{') &&
       (symbol_set[symbol_set.size() - 1] == '}')){
        
        std::cout << "CURLY BRACES NOT IMPLEMENTED" << std::endl;
        exit(1);
    }

    int index = 0;
    while(index < symbol_set.size()) {

        unsigned char c = symbol_set[index];

        //std::cout << "PROCESSING CHAR: " << c << std::endl;
        
        switch(c){
            
            // Brackets
        case '[' :
            if(escaped){
                column.set(c, value);
                if(range_set){
                    setRange(column,range_start,c,value);
                    range_set = false;
                }
                last_char = c;
                escaped = false;
            }else{
                bracket_sem++;
            }
            break;
        case ']' :
            if(escaped){
                column.set(c, value);
                if(range_set){
                    setRange(column,range_start,c,value);
                    range_set = false;
                }

                last_char = c;
            }else{
                bracket_sem--;
            }
            
            break;

            // Braces
        case '{' :
            
            //if(escaped){
                column.set(c, value);
                if(range_set){
                    setRange(column,range_start,c,value);
                    range_set = false;
                }
                
                last_char = c;
                //escaped = false;
                //}else{
                //brace_sem++;
                //}
            break;
        case '}' :
            //if(escaped){
                column.set(c, value);
                if(range_set){
                    setRange(column,range_start,c,value);
                    range_set = false;
                }
                last_char = c;
                //escaped = false;
                //}else{
                //brace_sem--;
                //}
            break;
            
            //escape
        case '\\' :
            if(escaped){
                column.set(c, value);
                if(range_set){
                    setRange(column,range_start,c,value);
                    range_set = false;
                }

                last_char = c;
                escaped = false;
            }else{
                escaped = true;
            }
            break;
            
            // escaped chars
        case 'n' :
            if(escaped){
                column.set('\n',value);
                if(range_set){
                    setRange(column,range_start,'\n',value);
                    range_set = false;
                }
                last_char = '\n';
                escaped = false;
            }else{
                column.set(c, value);
                if(range_set){
                    setRange(column,range_start,c,value);
                    range_set = false;
                }
                last_char = c;
            }
            break;
        case 'r' :
            if(escaped){
                column.set('\r',value);
                if(range_set){
                    setRange(column,range_start,'\r',value);
                    range_set = false;
                }
                last_char = '\r';
                escaped = false;
            }else{
                column.set(c, value);
                if(range_set){
                    setRange(column,range_start,'\r',value);
                    range_set = false;
                }
                last_char = c;
            }
            break;
        case 't' :
            if(escaped){
                column.set('\t',value);
                if(range_set){
                    setRange(column,range_start,'\r',value);
                    range_set = false;
                }
                last_char = '\t';
                escaped = false;
            }else{
                column.set(c, value);
                if(range_set){
                    setRange(column,range_start,c,value);
                    range_set = false;
                }
                last_char = c;
            }
            break;
        case 'a' :
            if(escaped){
                column.set('\a',value);
                if(range_set){
                    setRange(column,range_start,'\a',value);
                    range_set = false;
                }
                last_char = '\a';
                escaped = false;
            }else{
                column.set(c, value);
                if(range_set){
                    setRange(column,range_start,c,value);
                    range_set = false;
                }
                last_char = c;
            }
            break;
        case 'b' :
            if(escaped){
                column.set('\b',value);
                if(range_set){
                    setRange(column,range_start,'\b',value);
                    range_set = false;
                }
                last_char = '\b';
                escaped = false;
            }else{
                column.set(c, value);
                if(range_set){
                    //std::cout << "RANGE SET" << std::endl;
                    setRange(column,range_start,c,value);
                    range_set = false;
                }
                last_char = c;
            }
            break;
        case 'f' :
            if(escaped){
                column.set('\f',value);
                if(range_set){
                    setRange(column,range_start,'\f',value);
                    range_set = false;
                }
                last_char = '\f';
                escaped = false;
            }else{
                column.set(c, value);
                if(range_set){
                    setRange(column,range_start,c,value);
                    range_set = false;
                }
                last_char = c;
            }
            break;
        case 'v' :
            if(escaped){
                column.set('\v',value);
                if(range_set){
                    setRange(column,range_start,'\v',value);
                    range_set = false;
                }
                last_char = '\v';
                escaped = false;
            }else{
                column.set(c, value);
                if(range_set){
                    setRange(column,range_start,c,value);
                    range_set = false;
                }
                last_char = c;
            }
            break;
        case '\'' :
            if(escaped){
                column.set('\'',value);
                if(range_set){
                    setRange(column,range_start,'\'',value);
                    range_set = false;
                }
                last_char = '\'';
                escaped = false;
            }else{
                column.set(c, value);
                if(range_set){
                    setRange(column,range_start,c,value);
                    range_set = false;
                }
                last_char = c;
            }
            break;
        case '\"' :
            if(escaped){
                column.set('\"',value);
                if(range_set){
                    setRange(column,range_start,'\"',value);
                    range_set = false;
                }
                last_char = '\"';
                escaped = false;
            }else{
                column.set(c, value);
                if(range_set){
                    setRange(column,range_start,c,value);
                    range_set = false;
                }
                last_char = c;
            }
            break;
            /*
        case '?' :
            if(escaped){
                column.set('?',value);
                last_char = '?';
                escaped = false;
            }else{
                column.set(c, value);
                last_char = c;
            }
            break;
            */
            // Range
        case '-' :
            if(escaped){
                column.set('-',value);
                if(range_set){
                    setRange(column,range_start,'-',value);
                    range_set = false;
                }
                last_char = '-';
            }else{
                range_set = true;
                range_start = last_char;
            }
            break;

            // Special Classes
        case 's' :
            if(escaped){
                column.set('\n',value);
                column.set('\t',value);
                column.set('\r',value);
                column.set('\x0B',value); //vertical tab
                column.set('\x0C',value);
                column.set('\x20',value);
                escaped = false;
            }else{
                column.set(c, value);
                if(range_set){
                    setRange(column,range_start,c,value);
                    range_set = false;
                }
                last_char = c;
            }
            break;

        case 'd' :
            if(escaped){
                setRange(column,48,57, value);
                escaped = false;
            }else{
                column.set(c, value);
                if(range_set){
                    setRange(column,range_start,c,value);
                    range_set = false;
                }
                last_char = c;
            }
            break;

        case 'w' :
            if(escaped){
                column.set('_', value); // '_'
                setRange(column,48,57, value); // d
                setRange(column,65,90, value); // A-Z
                setRange(column,97,122, value); // a-z
                escaped = false;
            }else{
                column.set(c, value);
                if(range_set){
                    setRange(column,range_start,c,value);
                    range_set = false;
                }
                last_char = c;
            }
            break;

            // Inversion
        case '^' :
            if(escaped){
                column.set(c,value);
                if(range_set){
                    setRange(column,range_start,c,value);
                    range_set = false;
                }
                last_char = c;
                escaped = false;
            }else{
                inverting = true;
            }
            break;

            // HEX
        case 'x' :
            if(escaped){
                //process hex char
                ++index;
                char hex[3];
                hex[0] = (char)symbol_set.c_str()[index];
                hex[1] = (char)symbol_set.c_str()[index+1];
                hex[2] = '\0';
                unsigned char number = (unsigned char)std::strtoul(hex, NULL, 16);
                
                //
                ++index;
                column.set(number, value);
                if(range_set){
                    setRange(column,range_start,number,value);
                    range_set = false;
                }
                last_char = number;
                escaped = false;
            }else{
                column.set(c, value);
                if(range_set){
                    setRange(column,range_start,c,value);
                    range_set = false;
                }
                last_char = c;
            }
            break;


            // Other characters
        default:
            if(escaped){
                // we escaped a char that is not valid so treat it normaly
                escaped = false;
            }
            column.set(c, value);
            if(range_set){
                setRange(column,range_start,c,value);
                range_set = false;
            }
            last_char = c;
        };

        index++;
    } // char while loop

    if(inverting)
        column.flip();

    if(bracket_sem != 0 || 
       brace_sem != 0){
        std::cout << "MALFORMED BRACKETS OR BRACES: " << symbol_set <<  std::endl;
        std::cout << "brackets: " << bracket_sem << std::endl;
        exit(1);
    }


    /*    
    std::cout << "***" << std::endl;
    for(int i = 0; i < 256; i++){
        if(column.test(i))
            std::cout << i << " : 1" << std::endl;
        else
            std::cout << i << " : 0" << std::endl;
    }
    std::cout << "***" << std::endl;
    */
}


/*
 * Quine-Mcklusky Algorithm for calculating character-set complexity
 */
int count1s(size_t x) {
    int o = 0;
    while (x) {
        o += x % 2;
        x >>= 1;
    }
    return o;
}

/*
void printTab(const std::vector<Implicant> &imp) {
    for (size_t i = 0; i < imp.size(); ++i)
        //std::cout << imp[i] << std::endl;
    std::cout << "-------------------------------------------------------\n";
}
*/
void mul(std::vector<size_t> &a, const std::vector<size_t> &b) {

    std::vector<size_t> v;

    for (size_t i = 0; i < a.size(); ++i)
        for (size_t j = 0; j < b.size(); ++j)
            v.push_back(a[i] | b[j]);

    sort(v.begin(), v.end());
    v.erase( unique( v.begin(), v.end() ), v.end() );

    for (size_t i = 0; i < v.size() - 1; ++i)
        for (size_t j = v.size() - 1; j > i ; --j) {
            size_t z = v[i] & v[j];
            if ((z & v[i]) == v[i])
                v.erase(v.begin() + j);
            else if ((z & v[j]) == v[j]) {
                size_t t = v[i];
                v[i] = v[j];
                v[j] = t;
                v.erase(v.begin() + j);
                j = v.size();
            }
        }
    a = v;
}

int QMScore(std::bitset<256> column) {

    int combs = 8;
    //bool *minterms = (bool *)calloc(combs, sizeof(bool));
    std::vector<int> minterms;
    std::vector<Implicant> implicants;

    // Parse miniterms
    /*
    for (int mint; cin >> mint; ) {
        implicants.push_back(mint);
        minterms.push_back(mint);
    }
    */
    for(int i = 0; i < 255; i++){
        if(column[i]){
            implicants.push_back(i);
            minterms.push_back(i);
        }
    }

    if (!minterms.size()) { 
        //cout << "\n\tF = 0\n"; 
        return 0; 
    }

    std::sort(minterms.begin(), minterms.end());
    minterms.erase( unique( minterms.begin(), minterms.end() ), minterms.end() );

    // Parse not cares
    //if (!cin.eof() && cin.fail()) { // don't cares
    //    cin.clear();
    //    while ('d' != cin.get()) ;
    //    for (int mint; cin >> mint; )
    //        implicants.push_back(mint);
    //}

    std::sort(implicants.begin(), implicants.end());

    //printTab(implicants);

    std::vector<Implicant> aux;
    std::vector<Implicant> primes;

    while (implicants.size() > 1) {
        for (size_t i = 0; i < implicants.size() - 1; ++i)
            for (size_t j = implicants.size() - 1; j > i ; --j)
                if (implicants[j].bits == implicants[i].bits)
                    implicants.erase(implicants.begin() + j);
        aux.clear();
        for (size_t i = 0; i < implicants.size() - 1; ++i)
            for (size_t j = i + 1; j < implicants.size(); ++j)
                if (implicants[j].ones == implicants[i].ones + 1 &&
implicants[j].mask == implicants[i].mask &&
                    count1s(implicants[i].implicant ^
                            implicants[j].implicant) == 1) {
                    implicants[i].used = true;
                    implicants[j].used = true;
                    aux.push_back(
                                  Implicant(implicants[i].implicant,
                                            implicants[i].cat(implicants[j]),
implicants[i].minterms + ',' +
                                            implicants[j].minterms,
                                            (implicants[i].implicant ^
                                             implicants[j].implicant) | implicants[i].mask)
                                  );
                }
        for (size_t i = 0; i < implicants.size(); ++i)
            if (!implicants[i].used) primes.push_back(implicants[i]);
        implicants = aux;
        std::sort(implicants.begin(), implicants.end());
        //printTab(implicants);
    }

    for (size_t i = 0; i < implicants.size(); ++i)
        primes.push_back(implicants[i]);

    if (primes.back().mask == combs - 1){ 
        //std::cout << "\n\tF = 1\n"; 
        return 0; 
    }

    bool table[primes.size()][minterms.size()];

    for (size_t i = 0; i < primes.size(); ++i)
        for (size_t k = 0; k < minterms.size(); ++k)
            table[i][k] = false;

    for (size_t i = 0; i < primes.size(); ++i)
        for (size_t j = 0; j < primes[i].mints.size(); ++j)
            for (size_t k = 0; k < minterms.size(); ++k)
                if (primes[i].mints[j] == minterms[k])
                    table[i][k] = true;
    /*
    for (int k = 0; k < 18; ++k) 
        std::cout << " ";

    for (size_t k = 0; k < minterms.size(); ++k)
        std::cout << right << setw(2) << minterms[k] << ' ';
    std::cout << std::endl;
    for (int k = 0; k < 18; ++k) std::cout << " ";
    for (size_t k = 0; k < minterms.size(); ++k)
        std::cout << "---";
    std::cout << std::endl;
    for (size_t i = 0; i < primes.size(); ++i) {
        //std::cout << primes[i] << " |";
        for (size_t k = 0; k < minterms.size(); ++k)
            //std::cout << (table[i][k] ? " X " : " ");
        std::cout << std::endl;
    }
    */

    std::vector<size_t> M0, M1;
    for (size_t i = 0; i < primes.size(); ++i)
        if (table[i][0]) M0.push_back(1 << i);
    for (size_t k = 1; k < minterms.size(); ++k) {
        M1.clear();
        for (size_t i = 0; i < primes.size(); ++i)
            if (table[i][k]) M1.push_back(1 << i);
        mul(M0, M1);
    }

    int min = count1s(M0[0]);
    size_t ind = 0;
    // for (size_t i = 0; i < M0.size(); ++i) std::cout << M0[i] << ','; std::cout << std::endl;
    for (size_t i = 1; i < M0.size(); ++i){
        if (min > count1s(M0[i])) {
            min = count1s(M0[i]);
            ind = i;
        }
    }

    bool f;

    /*
    //std::cout << "-------------------------------------------------------\n";
    for (size_t j = 0; j < M0.size(); ++j) {
        //std::cout << "\tF = ";
        f = false;
        for (size_t i = 0; i < primes.size(); ++i)
            if (M0[j] & (1 << i)) {
                //if (f) std::cout << " + ";
                f = true;
                //std::cout << primes[i];
            }
        //std::cout << std::endl;
    }
    */
    //std::cout << "-------------------------------------------------------\n";
    
    // minimal solution
    //std::cout << "\n\tF = ";
    f = false;
    int term_count = 0;
    for (size_t i = 0; i < primes.size(); ++i)
        if (M0[ind] & (1 << i)) {
            //if (f) std::cout << " + ";
            f = true;
            //std::cout << primes[i];
            // calc number of not-cares
            size_t zeros = count(primes[i].bits.begin(), primes[i].bits.end(), '0');
            size_t ones = count(primes[i].bits.begin(), primes[i].bits.end(), '1');
            //std::cout << primes[i].bits << std::endl;;
            term_count += zeros + ones;
        }

    std::cout << "count: " << term_count << std::endl;
    //std::cout << std::endl;

    return term_count;
}

