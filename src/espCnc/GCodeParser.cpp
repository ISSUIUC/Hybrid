#include"GCodeParser.h"
#include"panic.h"
#include<cstdlib>

void GCodeParser::next_char(char c) {
    if(mode == ParseMode::FindToken) {
        if(c == '(') {
            mode = ParseMode::Comment;
        } else if(c == '%' || c == '\r' || c == '\n' || c == '\t' || c == ' ') {
            //ignore
        } else if(c >= 'A' && c <= 'Z') {
            current_letter = c;
            mode = ParseMode::ReadNumber;
        } else if(c >= 'a' && c <= 'z') {
            current_letter = c - 'a' + 'A';
            mode = ParseMode::ReadNumber;
        } else {
            Serial.print("Unexpected Token "); Serial.println(c);
            Serial.print("At parse location "); Serial.println(char_count);
            panic(15);
        }
    } else if(mode == ParseMode::ReadNumber) {
        if(c == ' ' || c == '\r' || c == '\n' || c == '\t') {
            process_cmd();
            mode = ParseMode::FindToken;
        } else {
            number_buff[number_idx++] = c;
        }
    } else if(mode == ParseMode::Comment) {
        if(c == ')') {
            mode = ParseMode::FindToken;
        }
    }
    char_count++;
}

void GCodeParser::process_cmd() {
    number_buff[number_idx] = 0;
    float num = std::atof(number_buff);
    char letter = current_letter;
    number_idx = 0;
    current_letter = 0;
    Token t{letter, num};
    next_token(t);
}

void GCodeParser::next_token(Token t) {
    if(t.letter == 'G' || t.letter == 'M'){
        process_tokens();
    }
    token_buff[token_idx++] = t;
}

void GCodeParser::process_tokens() {
    if(token_idx == 0) return;
    Token primary = token_buff[0];

    if(handler) {
        handler(primary, &token_buff[1], &token_buff[token_idx]);
    }

    token_idx = 0;
}