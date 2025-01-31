#include <iostream>
#include <vector>
#include <stack>
#include <cctype>
#include <string>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <arpa/inet.h>

using namespace std;


int precedence(char s) {
    if (s == '(') return -1;
    if (s == '+' || s == '-') return 0;
    if (s == '*' || s == '/' || s == '%') return 1;
    return -1;
}

vector<string> stringToStack(string s, int &operandCount) {
    vector<string> array;
    string temp = "";

    for (char c : s) {
        if (isspace(c)) {
            continue; // Ignore spaces
        } 
        else if (c == '+' || c == '-' || c == '*' || c == '/' || c == '%' || c== '(' || c==')') {
            if (!temp.empty()) {
                array.push_back(temp);
                temp = "";
                operandCount++; 
            }
            array.push_back(string(1, c)); 
        } 
        else {
            temp += c;
        }
    }

    if (!temp.empty()) {
        array.push_back(temp); 
        operandCount++; 
    }

    return array;
}


vector<string> infixToPostfix(vector<string>& tokens) {
    vector<string> postfix;
    stack<string> operators;

    for (const string& token : tokens) {
        if (isdigit(token[0])) {
            postfix.push_back(token);
        } 
        else if (token == "(") {
            operators.push(token);
        } 
        else if (token == ")") {
            while (!operators.empty() && operators.top() != "(") {
                postfix.push_back(operators.top());
                operators.pop();
            }
            if (!operators.empty()) operators.pop();
        } 
        else {
            while (!operators.empty() && precedence(operators.top()[0]) >= precedence(token[0])) {
                postfix.push_back(operators.top());
                operators.pop();
            }
            operators.push(token);
        }
    }

    while (!operators.empty()) {
        postfix.push_back(operators.top());
        operators.pop();
    }

    return postfix;
}

// Evaluate the postfix expression
int evaluatePostfix(vector<string>& postfix) {
    stack<int> values;

    for (const string& token : postfix) {
        if (isdigit(token[0])) {
            values.push(stoi(token));
        } 
        else {
            if (values.size() < 2) {
                cerr << "Error: Invalid postfix expression\n";
                return 0;
            }
            int b = values.top(); values.pop();
            int a = values.top(); values.pop();
            int result = 0;

            if (token == "+") result = a + b;
            else if (token == "-") result = a - b;
            else if (token == "*") result = a * b;
            else if (token == "/") {
                if (b == 0) {
                    cerr << "Error: Division by zero\n";
                    return 0;
                }
                result = a / b;
            }
            else if (token == "%") {
                if (b == 0) {
                    cerr << "Error: Modulo by zero\n";
                    return 0;
                }
                result = a % b;
            }

            values.push(result);
        }
    }

    return values.top();
}

int main() {
    int server_fd, new_socket;
    struct sockaddr_in address;
    int addrlen = sizeof(address);

    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0) {
        perror("Socket failed");
        exit(EXIT_FAILURE);
    }

    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(8080);

   
    if (bind(server_fd, (struct sockaddr*)&address, sizeof(address)) < 0) {
        perror("Bind failed");
        exit(EXIT_FAILURE);
    }

   
    if (listen(server_fd, 3) < 0) {
        perror("Listen failed");
        exit(EXIT_FAILURE);
    }

    cout << "Waiting for connections...\n";

    
    if ((new_socket = accept(server_fd, (struct sockaddr*)&address, (socklen_t*)&addrlen)) < 0) {
        perror("Accept failed");
        exit(EXIT_FAILURE);
    }

    char buffer[1024] = {0};
    while (true) {
        
        int valread = read(new_socket, buffer, 1024);
        if (valread == 0) break;  

        buffer[valread] = '\0';
        string infix(buffer);

        cout << "Infix Expression received: " << infix << endl;

        int operandCount = 0;
        vector<string> tokens = stringToStack(infix, operandCount);

        // If operand count exceeds 19, discard the expression
        if (operandCount > 19) {
            cout << "Expression discarded (more than 19 operands).\n";
            string resultStr = "Expression discarded (more than 19 operands).";
            send(new_socket, resultStr.c_str(), resultStr.length(), 0);
            continue; 
        }

        vector<string> postfix = infixToPostfix(tokens);

        int result = evaluatePostfix(postfix);

        
        string resultStr = to_string(result);
        send(new_socket, resultStr.c_str(), resultStr.length(), 0);
        cout << "Sent result: " << resultStr << endl;
    }

    close(new_socket);
    close(server_fd);
    return 0;
}
