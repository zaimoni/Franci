// (C)2022, license: LICENSE.md

#ifndef ZAIMONI_STL_OBSERVER_HPP
#define ZAIMONI_STL_OBSERVER_HPP 1

#include <functional>

namespace zaimoni {

	// copied from C#.
	template<class SRC>
	struct observer {
		virtual ~observer() = default;

		/// <returns>false if should be destructed as now irrelevant</returns>
		virtual bool onNext(const SRC& value) = 0; // observable reports data
	};

	template<class SRC>
	class lambda_observer : public observer<SRC>
	{
		std::function<bool(const SRC&)> _op;

	public:
		lambda_observer(decltype(_op) src) : _op(src) {}
		lambda_observer(const lambda_observer& src) = delete;
		lambda_observer(lambda_observer&& src) = delete;
		lambda_observer& operator=(const lambda_observer& src) = delete;
		lambda_observer& operator=(lambda_observer&& src) = delete;
		~lambda_observer() = default;

		bool onNext(const SRC& value) override { _op(value); }
	};
}

#endif

