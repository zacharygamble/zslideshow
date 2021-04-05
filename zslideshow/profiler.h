#pragma once

template<typename C, typename R, typename... Args>
R profile_invoke(R(C::*t)(Args...), const char* profile_name, Args... args)
{
	using std::chrono;
	auto start = chrono::high_resolution_clock::now();
	auto result = t(args...);
	auto end = chrono::high_resolution_clock::now();

	auto delta = end - start;
	cout << "Profile (" << profile_name << "): " << delta.count() << endl;

	return result;
}

template<typename R, typename... Args>
R profile_invoke(R(*t)(Args...), const char* profile_name, Args... args)
{
	using std::chrono;
	auto start = chrono::high_resolution_clock::now();
	auto result = t(args...);
	auto end = chrono::high_resolution_clock::now();

	auto delta = end - start;
	cout << "Profile (" << profile_name << "): " << delta.count() << endl;

	return result;
}
