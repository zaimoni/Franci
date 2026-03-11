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
	struct observed {
	protected:
		std::vector<std::weak_ptr<zaimoni::observer<SRC>>> _watchers;
	public:
		virtual ~observed() = default;

		virtual void watched_by(const std::shared_ptr<zaimoni::observer<SRC>>& src) = 0;

		void notify(const SRC& value) {
			ptrdiff_t ub = _watchers.size();
			while (0 <= --ub) {
				auto whom = _watchers[ub].lock();
				if (whom) {
					if (!whom->onNext(value)) {
						if (1 == _watchers.size()) {
							decltype(_watchers)().swap(_watchers);
						} else {
							_watchers[ub].swap(_watchers.back());
							_watchers.pop_back();
						}
					}
				} else if (1 == _watchers.size()) {
					decltype(_watchers)().swap(_watchers);
				} else {
					_watchers[ub].swap(_watchers.back());
					_watchers.pop_back();
				}
			}
		}

	protected:
		// Claude suggested bool return value
		bool seen_by(const std::shared_ptr<zaimoni::observer<SRC>>& src) {
			ptrdiff_t ub = _watchers.size();
			while (0 <= --ub) {
				auto whom = _watchers[ub].lock();
				if (whom) {
					if (whom == src) return false;
				}
				else if (1 == _watchers.size()) {
					decltype(_watchers)().swap(_watchers);
					return false;
				} else {
					_watchers[ub].swap(_watchers.back());
					_watchers.pop_back();
				}
			}
			_watchers.push_back(src);
			return true;
		}
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

		bool onNext(const SRC& value) override { return _op(value); }
	};
}

#endif

