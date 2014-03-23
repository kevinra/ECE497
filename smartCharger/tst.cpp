#include <iostream>
#include <iomanip>
#include <chrono>
#include <ctime>
#include <thread>

#ifdef DEBUG
  #define DBG_OUT_VAR(var) \
    std::cout << "DBG: " << __FILE__ << "(" << __LINE__ << ")::"\
         << __func__ << "> " << #var << " = [" << (var) << "]" << std::endl
  #define DBG_ERR_VAR(var) \
    std::cerr << "DBG: " << __FILE__ << "(" << __LINE__ << ") "\
         << #var << " = [" << (var) << "]" << std::endl
  #define DBG_OUT_MSG(msg) \
    std::cout << "DBG: " << __FILE__ << "(" << __LINE__ << ") " \
         << msg << std::endl
  #define DBG_ERR_MSG(msg) \
    std::cerr << "DBG: " << __FILE__ << "(" << __LINE__ << ") " \
         << msg << std::endl
#else
  #define DBG_OUT_VAR(var)
  #define DBG_ERR_VAR(var)
  #define DBG_OUT_MSG(msg)
  #define DBG_ERR_MSG(msg)
#endif

void timed_piece_of_code() 
{
    std::chrono::seconds simulated_work(2);
    std::this_thread::sleep_for(simulated_work);
}

int main()
{
  std::chrono::system_clock::time_point now = std::chrono::system_clock::now();
  std::time_t now_c = std::chrono::system_clock::to_time_t( now );
  std::cout << "One day ago, the time was "
            << std::put_time(std::localtime(&now_c), "%F %T") << '\n';


  std::cout << "Hello World\n";
  timed_piece_of_code();
  std::chrono::system_clock::time_point end = std::chrono::system_clock::now();

  // std::chrono::duration<float, std::chrono::milliseconds::period> diffT = std::chrono::duration<float, std::chrono::milliseconds::period> (end - now);
  std::chrono::duration<float> diffT = std::chrono::duration<float, std::chrono::milliseconds::period> (end - now);

  std::cout << "Printing took "
            << std::chrono::duration_cast<std::chrono::microseconds>(end - now).count()
            << std::endl
            << diffT.count()
            // << std::chrono::duration<float, std::chrono::milliseconds::period> (end - now).count()
            << "seconds.\n";

	return 0;
}

