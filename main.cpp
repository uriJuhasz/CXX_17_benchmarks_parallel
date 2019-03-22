#include <iostream>
#include <unordered_map>
#include <chrono>
#include <iomanip>
#include <algorithm>
#include <execution>

using namespace std;

template<class T>class Range final
{
public:
    typedef T Stride;

public:
    class Iterator final
    {
    public:
        explicit Iterator() : m_t(), m_stride() {}
        Iterator(const T& t, const T& stride) : m_t(t), m_stride(stride) {}

        T operator++() { m_t += m_stride; return m_t; }
        T operator++(int) { const auto r = m_t;  m_t += m_stride; return r; }

        T operator*() const { return m_t; }

        inline friend bool operator==(const Iterator& a, const Iterator& b) { return a.m_t == b.m_t; }
        inline friend bool operator!=(const Iterator& a, const Iterator& b) { return !(a == b); }

    private:
        T m_t;
        Stride m_stride;
    };

public:
    Range(const T& start, const T& end, const Stride& stride = static_cast<T>(1))
        : m_start(start), m_end(end), m_stride(stride)
    {}

    Iterator cbegin() const { return Iterator(m_start, m_stride); }
    Iterator cend  () const { return Iterator(m_end, m_stride); }

    Iterator begin() const { return Iterator(m_start, m_stride); }
    Iterator end() const { return Iterator(m_end, m_stride); }


private:
    T m_start, m_end;
    Stride m_stride;
};

template<class T> bool operator==(const typename Range<T>::Iterator& a, const typename Range<T>::Iterator& b)
{
    return a.m_t == b.m_t;
}

template<class T> bool operator!=(const typename Range<T>::Iterator& a, const typename Range<T>::Iterator& b)
{
    return !(a == b);
}

namespace std
{
    template<> class iterator_traits<typename Range<int>::Iterator>
    {
    public:
        typedef forward_iterator_tag iterator_category;
        typedef int difference_type;
    };
}

template<class F, class P0, class P1> void measureAndPrint(P0 p0, F f, P1 p1)
{
    p0();
    const auto startTime = chrono::high_resolution_clock::now();
    f();
    const auto endTime = chrono::high_resolution_clock::now();
    const auto msTime = chrono::duration_cast<chrono::milliseconds>(endTime - startTime).count();
    p1(msTime);
}

int main()
{
    cout << "Start" << endl;

    static const int numElements = 10000000;
    unordered_map<int, float> m;
    for (int i = 0; i < numElements; ++i)
        m[i] = static_cast<float>(i) + 1;
        
    cout << " Measuring lookups from " << typeid(m).name() << endl;

    static const int maxNumReads = 100000000;
    float x = 0.0f;
    for (auto numReads = 1 << 16; numReads < maxNumReads; numReads <<= 1)
    {
        measureAndPrint(
            [&numReads]() {cout << " " << setw(10) << numReads << " : "; },
            [&x,&m,&numReads]()
            {for (int i = 0; i < numReads; ++i)
            x += m[(i * 17) % numElements]; },
            [&numReads](const long long msTime) {
                const auto plTime = static_cast<float>(numReads) * 1000.0f / msTime;
                cout << setw(10) << msTime << "ms - " << fixed << setprecision(1) << setw(10) << plTime << "lu/s" << endl; 
            }
        );
        /*
        cout << " " << setw(10) << numReads << " : ";
        const auto startTime = chrono::high_resolution_clock::now();
        for (int i = 0; i < numReads; ++i)
            x += m[(i * 17) % numElements];
        const auto endTime = chrono::high_resolution_clock::now();
        const auto msTime = chrono::duration_cast<chrono::milliseconds>(endTime - startTime).count();
        const auto plTime = static_cast<float>(numReads) * 1000.0f / msTime;
        cout << setw(10) <<  msTime << "ms - " << fixed << setprecision(1) << setw(10) << plTime << "lu/s" << endl;
        */
    }

    cout << " Measuring parallel lookups from " << typeid(m).name() << endl;

    for (auto numReads = 1 << 16; numReads < maxNumReads; numReads <<= 1)
    {
        cout << " " << setw(10) << numReads << " : ";
        const auto startTime = chrono::high_resolution_clock::now();
        const auto range = Range(0, numReads, 1);
        for_each(execution::par,range.begin(), range.end(), [&x,&m](const int i)
            {
                x += m[(i * 17) % numElements];
            }
        );
        const auto endTime = chrono::high_resolution_clock::now();
        const auto msTime = chrono::duration_cast<chrono::milliseconds>(endTime - startTime).count();
        const auto plTime = static_cast<float>(numReads) * 1000.0f / msTime;
        cout << setw(10) << msTime << "ms - " << fixed << setprecision(1) << setw(10) << plTime << "lu/s" << endl;
    }

    if (x == 42.0)
        cout << "X=42" << endl;

    cout << "End" << endl;
}
