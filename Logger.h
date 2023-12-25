#pragma once
#define DEBUG  1
#if DEBUG == 1
#define LOG(x) std::cout<<  x <<std::endl;
#define LOGNOSTD(x) cout<<  x <<endl;
#else
#define LOG(x) 
#endif
#define SQLDEBUG 1

#if SQLDEBUG == 1
#define LOGSQL(x) std::cout<<  x <<std::endl;
#define LOGSQLNOSTD(x) cout<<  x <<endl;
#else
#define LOGSQL(x) 
#endif
