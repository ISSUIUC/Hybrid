#pragma once

enum class ParseMode {
    Comment,
    FindToken,
    ReadNumber
};

struct Token {
    char letter;
    float number;
};

typedef void (*HandleCommand)(Token primary, Token* params_beg, Token* params_end);

struct GCodeParser
{
public:
    GCodeParser() {

    }

    void next_char(char c);
    void set_callback(HandleCommand handler) {
        this->handler = handler;
    }
private:
    void process_cmd();
    void process_tokens();
    void next_token(Token t);
    ParseMode mode = ParseMode::FindToken;
    char current_letter = 0;
    char number_buff[256];
    int number_idx = 0;
    Token token_buff[32];
    int token_idx = 0;
    HandleCommand handler;
    int char_count = 0;
};
