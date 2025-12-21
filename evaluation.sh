#!/bin/bash

# ==============================================================================
# WEBSERV EVALUATION SCRIPT - Adapted for your project
# ==============================================================================
# Based on official evaluation scale - adapted for:
# - poll() I/O multiplexing
# - Port 8080
# - webserv.conf configuration
# - YoupyBanane directory structure
# - ubuntu_cgi_tester for .bla files
# ==============================================================================

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
PURPLE='\033[0;35m'
CYAN='\033[0;36m'
WHITE='\033[1;37m'
NC='\033[0m' # No Color

# Global variables
TOTAL_POINTS=0
MAX_POINTS=0
WEBSERV_PID=""
LOG_FILE="evaluation_results.log"
PORT=8080
CONFIG_FILE="configs/evaluation.conf"

# Initialize log file
echo "WEBSERV EVALUATION LOG - $(date)" > $LOG_FILE
echo "========================================" >> $LOG_FILE

# Simple cleanup function
cleanup_on_exit() {
    echo ""
    echo -e "${CYAN}🧹 Limpieza...${NC}"
    
    if [ -n "$WEBSERV_PID" ]; then
        kill $WEBSERV_PID 2>/dev/null || true
    fi
    killall -9 webserv 2>/dev/null || true
    killall curl 2>/dev/null || true
    sleep 1
    
    rm -f test_upload.txt test_download.txt temp_upload_file.txt test_file_upload.txt temp_put_file.txt temp_delete.txt 2>/dev/null || true
    
    echo -e "${GREEN}✅ Limpieza completada${NC}"
}

trap cleanup_on_exit EXIT

echo -e "${CYAN}=========================================="
echo -e "    🔍 WEBSERV EVALUATION - 42 SCHOOL"
echo -e "==========================================${NC}"
echo ""

# Cleanup function for hanging processes
cleanup_hanging_processes() {
    echo -e "${CYAN}🧹 Cleaning up any hanging processes...${NC}"
    
    pkill -f "curl.*localhost:$PORT" 2>/dev/null || true
    killall -9 webserv 2>/dev/null || true
    lsof -ti:$PORT 2>/dev/null | xargs -r kill -9 2>/dev/null || true
    
    sleep 2
    echo -e "${GREEN}✅ Cleanup completed${NC}"
}

# Helper functions
print_header() {
    echo -e "\n${PURPLE}============================================================${NC}"
    echo -e "${WHITE}$1${NC}"
    echo -e "${PURPLE}============================================================${NC}\n"
}

print_section() {
    echo -e "\n${BLUE}🔍 $1${NC}"
    echo -e "${CYAN}------------------------------------------------------------${NC}"
}

print_test() {
    echo -e "${YELLOW}📋 TEST: $1${NC}"
}

print_result() {
    if [ "$1" = "PASS" ]; then
        echo -e "${GREEN}✅ RESULT: $2${NC}"
        ((TOTAL_POINTS++))
    elif [ "$1" = "FAIL" ]; then
        echo -e "${RED}❌ RESULT: $2${NC}"
    else
        echo -e "${YELLOW}⚠️  RESULT: $2${NC}"
    fi
    ((MAX_POINTS++))
    echo "[$1] $2" >> $LOG_FILE
}

check_process() {
    if [ -n "$WEBSERV_PID" ] && kill -0 $WEBSERV_PID 2>/dev/null; then
        return 0
    else
        return 1
    fi
}

start_server() {
    echo -e "${CYAN}🚀 Starting webserv server...${NC}"
    
    killall -9 webserv 2>/dev/null || true
    sleep 1
    
    lsof -ti:$PORT 2>/dev/null | xargs -r kill -9 2>/dev/null || true
    sleep 1
    
    ./webserv $CONFIG_FILE > server_eval.log 2>&1 &
    WEBSERV_PID=$!
    
    sleep 3
    
    if check_process; then
        echo -e "${GREEN}✅ Server started with PID: $WEBSERV_PID${NC}"
        return 0
    else
        echo -e "${RED}❌ Failed to start server${NC}"
        if [ -f server_eval.log ]; then
            echo -e "${RED}Server log (last 20 lines):${NC}"
            tail -20 server_eval.log
        fi
        return 1
    fi
}

stop_server() {
    if [ -n "$WEBSERV_PID" ] && check_process; then
        echo -e "${CYAN}🛑 Stopping webserv server (PID: $WEBSERV_PID)...${NC}"
        kill $WEBSERV_PID 2>/dev/null
        wait $WEBSERV_PID 2>/dev/null
        WEBSERV_PID=""
        sleep 1
    fi
    
    killall -9 webserv 2>/dev/null || true
    lsof -ti:$PORT 2>/dev/null | xargs -r kill -9 2>/dev/null || true
    sleep 1
}

# ==============================================================================
# INTRODUCTION AND SETUP
# ==============================================================================

print_header "WEBSERV PROJECT EVALUATION"

echo -e "${WHITE}Evaluating project with:${NC}"
echo -e "${WHITE}  - Config: $CONFIG_FILE${NC}"
echo -e "${WHITE}  - Port: $PORT${NC}"
echo -e "${WHITE}  - I/O Multiplexing: poll()${NC}\n"

cleanup_hanging_processes

# Check prerequisites
print_section "Prerequisites Check"

print_test "Checking if project compiles"
make clean > /dev/null 2>&1
if make > compile.log 2>&1; then
    print_result "PASS" "Project compiles successfully without warnings/errors"
else
    print_result "FAIL" "Project does not compile - EVALUATION ENDS HERE"
    echo -e "${RED}COMPILATION ERRORS:${NC}"
    cat compile.log
    exit 1
fi

print_test "Checking webserv executable exists"
if [ -f "./webserv" ]; then
    print_result "PASS" "webserv executable found"
else
    print_result "FAIL" "webserv executable not found"
    exit 1
fi

print_test "Checking configuration file exists"
if [ -f "$CONFIG_FILE" ]; then
    print_result "PASS" "Configuration file $CONFIG_FILE found"
elif [ -f "configs/$CONFIG_FILE" ]; then
    CONFIG_FILE="configs/$CONFIG_FILE"
    print_result "PASS" "Configuration file $CONFIG_FILE found"
else
    print_result "FAIL" "Configuration file not found"
    exit 1
fi

print_test "Checking YoupyBanane directory structure"
if [ -d "YoupyBanane" ] && [ -f "YoupyBanane/youpi.bla" ] && [ -f "YoupyBanane/youpi.bad_extension" ]; then
    print_result "PASS" "YoupyBanane directory structure correct"
else
    print_result "FAIL" "YoupyBanane directory structure missing"
fi

print_test "Checking ubuntu_cgi_tester exists"
if [ -f "./ubuntu_cgi_tester" ] && [ -x "./ubuntu_cgi_tester" ]; then
    print_result "PASS" "ubuntu_cgi_tester found and executable"
else
    print_result "WARNING" "ubuntu_cgi_tester not found or not executable"
fi

# ==============================================================================
# MANDATORY PART - CHECK THE CODE
# ==============================================================================

print_header "MANDATORY PART - CODE INSPECTION"

print_section "1. I/O Multiplexing Analysis"

print_test "Checking I/O Multiplexing function used"
if grep -rq "poll(" sources/; then
    print_result "PASS" "Uses poll() for I/O multiplexing"
    echo -e "${CYAN}📝 EXPLANATION: poll() monitors multiple file descriptors simultaneously${NC}"
elif grep -rq "select(" sources/; then
    print_result "PASS" "Uses select() for I/O multiplexing"
elif grep -rq "epoll" sources/; then
    print_result "PASS" "Uses epoll() for I/O multiplexing"
else
    print_result "FAIL" "No I/O multiplexing function found"
fi

print_test "Checking if poll()/select() is in main loop"
if grep -A 20 -B 5 "while" sources/ServerManager.cpp 2>/dev/null | grep -q "poll\|select"; then
    print_result "PASS" "I/O multiplexing in main loop"
else
    print_result "WARNING" "Manual verification needed for main loop"
fi

print_test "Checking poll() monitors read AND write (POLLIN/POLLOUT)"
if grep -rq "POLLIN" sources/ && grep -rq "POLLOUT" sources/; then
    print_result "PASS" "poll() monitors both read (POLLIN) and write (POLLOUT) events"
else
    print_result "WARNING" "Check POLLIN/POLLOUT usage manually"
fi

print_test "Checking error handling for recv/send operations"
recv_check=$(grep -rc "recv\|read" sources/*.cpp 2>/dev/null | awk -F: '{sum+=$2} END {print sum}')
send_check=$(grep -rc "send\|write" sources/*.cpp 2>/dev/null | awk -F: '{sum+=$2} END {print sum}')

if [ "${recv_check:-0}" -gt 0 ] && [ "${send_check:-0}" -gt 0 ]; then
    print_result "PASS" "recv/send operations found in code"
else
    print_result "WARNING" "Check recv/send usage manually"
fi

print_test "Checking for FORBIDDEN errno usage after recv/send"
errno_after_recv=$(grep -A 3 "recv(" sources/*.cpp 2>/dev/null | grep -v "//" | grep -c "errno" || echo "0")
errno_after_send=$(grep -A 3 "send(" sources/*.cpp 2>/dev/null | grep -v "//" | grep -c "errno" || echo "0")

if [ "${errno_after_recv:-0}" -eq 0 ] && [ "${errno_after_send:-0}" -eq 0 ]; then
    print_result "PASS" "No errno usage after recv/send operations"
else
    print_result "WARNING" "Check errno usage manually - may be acceptable context"
fi

# ==============================================================================
# MANDATORY PART - CONFIGURATION TESTING
# ==============================================================================

print_header "MANDATORY PART - CONFIGURATION TESTING"

print_section "2. Server Configuration Tests"

print_test "Starting server for configuration tests"
if start_server; then
    print_result "PASS" "Server started successfully"
else
    print_result "FAIL" "Server failed to start"
    exit 1
fi

print_test "Checking server is listening on port $PORT"
sleep 2
response=$(curl -s -o /dev/null -w "%{http_code}" --max-time 5 http://localhost:$PORT/ 2>/dev/null)
if echo "$response" | grep -q "200\|404\|403"; then
    print_result "PASS" "Server responding on port $PORT (code: $response)"
else
    print_result "FAIL" "Server not responding on port $PORT"
fi

print_test "Testing GET request on /"
get_response=$(curl -s -o /dev/null -w "%{http_code}" --max-time 5 http://localhost:$PORT/ 2>/dev/null)
if [ "$get_response" = "200" ]; then
    print_result "PASS" "GET / returns 200 OK"
else
    print_result "WARNING" "GET / returns $get_response (expected 200)"
fi

print_test "Testing 404 error page"
error_response=$(curl -s -o /dev/null -w "%{http_code}" --max-time 5 http://localhost:$PORT/nonexistent-page-xyz 2>/dev/null)
if [ "$error_response" = "404" ]; then
    print_result "PASS" "404 error page working"
else
    print_result "FAIL" "404 not returned (got: $error_response)"
fi

print_test "Testing method restrictions (POST on / should fail)"
post_root=$(curl -s -o /dev/null -w "%{http_code}" -X POST --max-time 5 http://localhost:$PORT/ 2>/dev/null)
if [ "$post_root" = "405" ]; then
    print_result "PASS" "POST on / returns 405 Method Not Allowed"
else
    print_result "WARNING" "POST on / returns $post_root (expected 405)"
fi

# ==============================================================================
# MANDATORY PART - BASIC HTTP METHODS
# ==============================================================================

print_header "MANDATORY PART - HTTP METHODS"

print_section "3. HTTP Methods Testing"

print_test "Testing GET on /directory/"
dir_response=$(curl -s -o /dev/null -w "%{http_code}" --max-time 5 http://localhost:$PORT/directory/ 2>/dev/null)
if [ "$dir_response" = "200" ]; then
    print_result "PASS" "GET /directory/ returns 200"
else
    print_result "WARNING" "GET /directory/ returns $dir_response"
fi

print_test "Testing autoindex on /directory/"
dir_content=$(curl -s --max-time 5 http://localhost:$PORT/directory/ 2>/dev/null)
if echo "$dir_content" | grep -qi "youpi\|index\|directory\|listing\|href"; then
    print_result "PASS" "Directory listing or index file served"
else
    print_result "WARNING" "Directory content may not be as expected"
fi

print_test "Testing GET on /uploads/ (autoindex)"
uploads_response=$(curl -s -o /dev/null -w "%{http_code}" --max-time 5 http://localhost:$PORT/uploads/ 2>/dev/null)
if [ "$uploads_response" = "200" ]; then
    print_result "PASS" "GET /uploads/ returns 200 (autoindex)"
else
    print_result "WARNING" "GET /uploads/ returns $uploads_response"
fi

print_test "Testing PUT request on /put_test/"
echo "Test PUT content $(date)" > temp_put_file.txt
put_response=$(curl -s -X PUT -T temp_put_file.txt -o /dev/null -w "%{http_code}" --max-time 5 http://localhost:$PORT/put_test/test_file.txt 2>/dev/null)
rm -f temp_put_file.txt

if [ "$put_response" = "200" ] || [ "$put_response" = "201" ] || [ "$put_response" = "204" ]; then
    print_result "PASS" "PUT request working ($put_response)"
else
    print_result "WARNING" "PUT request returned $put_response"
fi

print_test "Testing DELETE request on /uploads/"
# First create a file to delete
echo "Delete test" | curl -s -X PUT -T - http://localhost:$PORT/put_test/delete_me.txt >/dev/null 2>&1
sleep 1

delete_response=$(curl -s -X DELETE -o /dev/null -w "%{http_code}" --max-time 5 http://localhost:$PORT/uploads/delete_me.txt 2>/dev/null)
if [ "$delete_response" = "200" ] || [ "$delete_response" = "204" ]; then
    print_result "PASS" "DELETE request working ($delete_response)"
else
    print_result "WARNING" "DELETE returned $delete_response"
fi

print_test "Testing client body size limit on /post_body (100 bytes)"
large_body=$(python3 -c "print('A' * 200)" 2>/dev/null || printf 'A%.0s' {1..200})
body_limit_response=$(curl -s -X POST -H "Content-Type: text/plain" --data "$large_body" -o /dev/null -w "%{http_code}" --max-time 5 http://localhost:$PORT/post_body 2>/dev/null)

if [ "$body_limit_response" = "413" ]; then
    print_result "PASS" "Client body size limit working (413 Payload Too Large)"
else
    print_result "WARNING" "Body limit test returned $body_limit_response (expected 413)"
fi

print_test "Testing server stability with unknown method"
unknown_response=$(curl -s -X PATCH -o /dev/null -w "%{http_code}" --max-time 5 http://localhost:$PORT/ 2>/dev/null)
sleep 1
if check_process; then
    print_result "PASS" "Server survived unknown method (response: $unknown_response)"
else
    print_result "FAIL" "Server crashed on unknown method"
fi

# ==============================================================================
# MANDATORY PART - CGI TESTING
# ==============================================================================

print_header "MANDATORY PART - CGI FUNCTIONALITY"

print_section "4. CGI Testing (.bla files)"

print_test "Testing CGI with POST to /directory/youpi.bla"
cgi_response=$(curl -s -X POST --data "test=data" --max-time 10 http://localhost:$PORT/directory/youpi.bla 2>/dev/null)
cgi_code=$(curl -s -o /dev/null -w "%{http_code}" -X POST --data "test=data" --max-time 10 http://localhost:$PORT/directory/youpi.bla 2>/dev/null)

echo -e "${CYAN}   CGI Response code: $cgi_code${NC}"
echo -e "${CYAN}   CGI Response: $(echo "$cgi_response" | head -c 100)${NC}"

if [ "$cgi_code" = "200" ] && ! echo "$cgi_response" | grep -qi "incorrect\|error"; then
    print_result "PASS" "CGI working with POST to .bla file"
elif [ "$cgi_code" = "200" ]; then
    print_result "WARNING" "CGI returns 200 but may have issues"
else
    print_result "FAIL" "CGI not working properly (code: $cgi_code)"
fi

print_test "Testing server stability after CGI"
sleep 1
if check_process; then
    print_result "PASS" "Server stable after CGI execution"
else
    print_result "FAIL" "Server crashed after CGI"
fi

print_test "Testing Python CGI in /cgi-bin/ (if configured)"
if [ -f "cgi-bin/test.py" ]; then
    python_cgi=$(curl -s --max-time 5 http://localhost:$PORT/cgi-bin/test.py 2>/dev/null)
    python_code=$(curl -s -o /dev/null -w "%{http_code}" --max-time 5 http://localhost:$PORT/cgi-bin/test.py 2>/dev/null)
    if [ "$python_code" = "200" ]; then
        print_result "PASS" "Python CGI accessible"
    else
        print_result "WARNING" "Python CGI returned $python_code"
    fi
else
    print_result "WARNING" "No Python CGI script found in cgi-bin/"
fi

# ==============================================================================
# STRESS TESTING
# ==============================================================================

print_header "STRESS TESTING"

print_section "5. Performance and Stability"

print_test "Testing concurrent connections"
echo -e "${CYAN}   Sending 20 concurrent requests...${NC}"

pids=()
for i in {1..20}; do
    timeout 5 curl -s http://localhost:$PORT/ >/dev/null 2>&1 &
    pids+=($!)
done

sleep 4
for pid in "${pids[@]}"; do
    kill $pid 2>/dev/null || true
done

sleep 2

if check_process; then
    print_result "PASS" "Server handles concurrent connections"
else
    print_result "FAIL" "Server crashed under concurrent load"
fi

print_test "Testing rapid sequential requests"
SUCCESS_COUNT=0
for i in {1..30}; do
    if curl -s --max-time 2 -o /dev/null http://localhost:$PORT/ 2>/dev/null; then
        ((SUCCESS_COUNT++))
    fi
done

AVAILABILITY=$((SUCCESS_COUNT * 100 / 30))
echo -e "${CYAN}   Successful: $SUCCESS_COUNT/30 (${AVAILABILITY}%)${NC}"

if [ "$AVAILABILITY" -gt 80 ]; then
    print_result "PASS" "Sequential requests: ${AVAILABILITY}% availability"
elif [ "$AVAILABILITY" -gt 50 ]; then
    print_result "WARNING" "Sequential requests: ${AVAILABILITY}% availability"
else
    print_result "FAIL" "Sequential requests: ${AVAILABILITY}% availability"
fi

# ==============================================================================
# UBUNTU TESTER
# ==============================================================================

print_header "UBUNTU TESTER"

print_section "6. Running ubuntu_tester"

if [ -f "./ubuntu_tester" ] && [ -x "./ubuntu_tester" ]; then
    print_test "ubuntu_tester available"
    echo -e "${YELLOW}   Note: ubuntu_tester requires manual interaction${NC}"
    echo -e "${CYAN}   To run manually:${NC}"
    echo -e "${WHITE}   ./ubuntu_tester $CONFIG_FILE http://localhost:$PORT${NC}"
    print_result "WARNING" "ubuntu_tester requires manual execution"
else
    print_result "WARNING" "ubuntu_tester not available"
fi

# ==============================================================================
# CLEANUP AND FINAL RESULTS
# ==============================================================================

print_header "EVALUATION COMPLETE"

stop_server

# Calculate final score
if [ $MAX_POINTS -gt 0 ]; then
    PERCENTAGE=$((TOTAL_POINTS * 100 / MAX_POINTS))
else
    PERCENTAGE=0
fi

echo -e "\n${WHITE}📊 EVALUATION SUMMARY${NC}"
echo -e "${WHITE}===========================================${NC}"
echo -e "${CYAN}Total Points: ${WHITE}$TOTAL_POINTS${CYAN} / ${WHITE}$MAX_POINTS${NC}"
echo -e "${CYAN}Percentage: ${WHITE}$PERCENTAGE%${NC}"

if [ $PERCENTAGE -ge 80 ]; then
    echo -e "${GREEN}🎉 RESULT: EXCELLENT PROJECT${NC}"
elif [ $PERCENTAGE -ge 60 ]; then
    echo -e "${YELLOW}✅ RESULT: GOOD PROJECT${NC}"
else
    echo -e "${RED}⚠️  RESULT: NEEDS IMPROVEMENT${NC}"
fi

echo -e "\n${CYAN}📝 Log saved to: ${WHITE}$LOG_FILE${NC}"
echo -e "${CYAN}📝 Server log: ${WHITE}server_eval.log${NC}"

echo -e "\n${PURPLE}============================================================${NC}"
echo -e "${WHITE}EVALUATION COMPLETE - $(date)${NC}"
echo -e "${PURPLE}============================================================${NC}"

echo -e "\n${YELLOW}💡 NOTES FOR EVALUATOR:${NC}"
echo -e "${YELLOW}   - Run ubuntu_tester manually for full CGI tests:${NC}"
echo -e "${WHITE}     ./ubuntu_tester $CONFIG_FILE http://localhost:$PORT${NC}"
echo -e "${YELLOW}   - Check server_eval.log for runtime info${NC}"
echo -e "${YELLOW}   - Verify code manually for edge cases${NC}"

exit 0
