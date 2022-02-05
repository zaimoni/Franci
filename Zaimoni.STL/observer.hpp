// (C)2022, license: LICENSE.md

#ifndef ZAIMONI_STL_OBSERVER_HPP
#define ZAIMONI_STL_OBSERVER_HPP 1

#include <functional>

namespace zaimoni {

	// copied from C#.
	template<class SRC>
	struct observer {
		virtual void onNext(const SRC& value) = 0; // observable reports data
	};

	template<class SRC>
	class lambda_observer : public observer<SRC>
	{
		std::function<void(const SRC&)> _op;

	public:
		lambda_observer(decltype(_op) src) : _op(src) {}
		lambda_observer(const lambda_observer& src) = delete;
		lambda_observer(lambda_observer&& src) = delete;
		lambda_observer& operator=(const lambda_observer& src) = delete;
		lambda_observer& operator=(lambda_observer&& src) = delete;
		~lambda_observer() = default;

		void onNext(const SRC& value) override { _op(value); }
	};
}

#endif

