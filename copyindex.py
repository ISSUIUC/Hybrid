index_str = open("./src/espCnc/web/public/index.html").read();
js_str = open("./src/espCnc/web/public/bundle.js").read();
open("./src/espCnc/web/index_string.h","w").write(
    f"#pragma once\nstatic constexpr const char * index_string = R\"rawstring({index_str})rawstring\";\nstatic constexpr const char * js_string = R\"rawstring({js_str})rawstring\";\n"
);
