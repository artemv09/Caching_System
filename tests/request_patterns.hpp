#ifndef REQUEST_PATTERNS_HPP
#define REQUEST_PATTERNS_HPP

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <sstream>
#include <iomanip>
#include <limits>
#include <optional>
#include <random>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

#include "creat_cach.hpp"

namespace request_patterns
{
struct Pattern
{
    std::string name;
    std::vector<int> keys;
    bool randomized = false;
    std::optional<std::uint32_t> seed;
};

// Проверяет границы до преобразований в int и выделения больших массивов.
inline void check_range(std::size_t count, std::size_t width)
{
    if(count > MAX_REQUESTS || width == 0 || width > MAX_PAGE_KEY)
    {
        throw std::invalid_argument("слишком большое число");
    }
}

// Добавляет не более limit запросов: first, first+1, ..., first+width-1, ...
inline void append_cycle(std::vector<int>& out, std::size_t limit, int first, std::size_t width,
                         std::size_t count)
{
    check_range(count, width);

    if(first < 1 || static_cast<std::size_t>(first) + width - 1 > MAX_PAGE_KEY || limit > MAX_REQUESTS)
    {
        throw std::invalid_argument("Диапазон цикла выходит за границы ключей");
    }

    if(width == 0)
    {
        throw std::invalid_argument("");
    }

    for(std::size_t i = 0; i < count && out.size() < limit; ++i)
    {
        out.push_back(first + static_cast<int>(i % width));
    }
}

// 1, 2, ..., W, 1, 2, ..., W, ...
inline std::vector<int> cycle(std::size_t count, std::size_t width)
{
    check_range(count, width);

    std::vector<int> out;
    out.reserve(count);

    append_cycle(out, count, 1, width, count);
    return out;
}

// Те же количества ключей, что в cycle, но одинаковые ключи идут подряд.
inline std::vector<int> grouped(std::size_t count, std::size_t width)
{
    auto keys = cycle(count, width);
    std::sort(keys.begin(), keys.end());
    return keys;
}

// Равномерные независимые запросы к W ключам.
inline std::vector<int> uniform(std::size_t count, std::size_t width, std::uint32_t seed)
{
    check_range(count, width);

    if(width == 0)
    {
        throw std::invalid_argument("uniform width must be positive");
    }

    std::mt19937 rng(seed);
    std::uniform_int_distribution<int> pick(1, static_cast<int>(width));

    std::vector<int> out(count);

    for(int& key : out)
    {
        key = pick(rng);
    }
    return out;
}

// На каждом проходе W ключей заново перемешиваются; повторов внутри прохода нет.
inline std::vector<int> shuffled_cycles(std::size_t count, std::size_t width, std::uint32_t seed)
{
    check_range(count, width);

    if(width == 0)
    {
        throw std::invalid_argument("shuffle width must be positive");
    }
    std::mt19937 rng(seed);
    auto block = cycle(width, width);

    std::vector<int> out;
    out.reserve(count);
    while(out.size() < count)
    {
        std::shuffle(block.begin(), block.end(), rng);

        for(int key : block)
        {
            if(out.size() == count)
            {
                break;
            }
            out.push_back(key);
        }
    }
    return out;
}

// 1, 2, ..., W, W-1, ..., 2, 1, 2, ...
inline std::vector<int> ping_pong(std::size_t count, std::size_t width)
{
    check_range(count, width);

    if(width < 2)
    {
        throw std::invalid_argument("ping-pong width must be at least 2");
    }
    std::vector<int> out(count);
    const std::size_t period = 2 * (width - 1);

    for(std::size_t i = 0; i < count; ++i)
    {
        const std::size_t pos = i % period;
        out[i] = 1 + static_cast<int>(pos < width ? pos : period - pos);
    }
    return out;
}

// 1,1,..., 2,2,...; каждый новый ключ используется burst раз и больше не приходит.
inline std::vector<int> bursts(std::size_t count, std::size_t burst)
{
    check_range(count, burst);

    if(burst == 0)
    {
        throw std::invalid_argument("burst must be positive");
    }

    std::vector<int> out(count);

    for(std::size_t i = 0; i < count; ++i)
    {
        out[i] = 1 + static_cast<int>(i / burst);
    }
    return out;
}

// Два прохода по группе, затем новая непересекающаяся группа.
// Например W=3: 1,2,3,1,2,3,4,5,6,4,5,6,...
inline std::vector<int> delayed_pairs(std::size_t count, std::size_t width)
{
    check_range(count, width);

    if(width == 0)
    {
        throw std::invalid_argument("pair width must be positive");
    }
    std::vector<int> out(count);
    for(std::size_t i = 0; i < count; ++i)
    {
        out[i] = 1 + static_cast<int>((i / (2 * width)) * width + i % width);
    }
    return out;
}

// p процентов запросов приходятся на hot ключей, остальные — на cold других ключей.
inline std::vector<int> hot_cold(std::size_t count, std::size_t hot, std::size_t cold, int p,
                                 std::uint32_t seed)
{
    check_range(count, hot);
    check_range(count, cold);
    if(hot + cold > MAX_PAGE_KEY)
    {
        throw std::invalid_argument("Слишком большие множества");
    }

    if(hot == 0 || cold == 0 || p < 0 || p > 100)
    {
        throw std::invalid_argument("invalid hot/cold parameters");
    }
    std::mt19937 rng(seed);
    std::uniform_int_distribution<int> percent(0, 99);
    std::uniform_int_distribution<int> hot_key(1, static_cast<int>(hot));
    std::uniform_int_distribution<int> cold_key(static_cast<int>(hot + 1), static_cast<int>(hot + cold));

    std::vector<int> out(count);

    for(int& key : out)
    {
        key = percent(rng) < p ? hot_key(rng) : cold_key(rng);
    }
    return out;
}

// Длинная серия обращений к горячему набору, затем проход по новым ключам.
// Каждый проход использует новые холодные ключи; горячий набор сохраняется.
inline std::vector<int> hot_scan(std::size_t count, std::size_t hot, std::size_t scan_length)
{
    check_range(count, hot);
    check_range(count, scan_length);
    // Новые ключи появляются только при сканировании, а не в горячих повторах.
    const std::size_t hot_length = 8 * hot;
    const std::size_t block_length = hot_length + scan_length;
    const std::size_t remainder = count % block_length;
    const std::size_t cold_count = (count / block_length) * scan_length +
                                  (remainder > hot_length ? remainder - hot_length : 0);
    if(cold_count > MAX_PAGE_KEY - hot)
    {
        throw std::invalid_argument("Сканирование выходит за диапазон ключей");
    }

    if(hot == 0 || scan_length == 0)
    {
        throw std::invalid_argument("invalid hot/scan parameters");
    }
    std::vector<int> out;
    out.reserve(count);
    int next_cold = static_cast<int>(hot + 1);

    while(out.size() < count)
    {
        append_cycle(out, count, 1, hot, std::min(hot_length, count - out.size()));
        for(std::size_t i = 0; i < scan_length && out.size() < count; ++i)
        {
            out.push_back(next_cold++);
        }
    }
    return out;
}

// Фазы из непересекающихся наборов: A -> B -> C... либо A -> B -> A -> B...
inline std::vector<int> phases(std::size_t count, std::size_t width, std::size_t phase_length,
                               bool return_to_old)
{
    check_range(count, width);
    check_range(count, phase_length);

    if((return_to_old ? 2 : (count == 0 ? 1 : (count - 1) / phase_length + 1)) * width > MAX_PAGE_KEY)
    {
        throw std::invalid_argument("Фазы выходят за диапазон ключей");
    }

    if(width == 0 || phase_length == 0)
    {
        throw std::invalid_argument("invalid phase parameters");
    }

    std::vector<int> out(count);

    for(std::size_t i = 0; i < count; ++i)
    {
        std::size_t phase = i / phase_length;
        if(return_to_old)
        {
            phase %= 2;
        }
        out[i] = 1 + static_cast<int>(phase * width + (i % phase_length) % width);
    }
    return out;
}

// Равномерные запросы внутри окна, которое постепенно смещается.
inline std::vector<int> moving_window(std::size_t count, std::size_t width, std::size_t phase_length,
                                      std::uint32_t seed)
{
    check_range(count, width);
    check_range(count, phase_length);

    if((count == 0 ? 0 : (count - 1) / phase_length) * std::max<std::size_t>(1, width / 4) + width > MAX_PAGE_KEY)
    {
        throw std::invalid_argument("Окно выходит за диапазон ключей");
    }

    if(width == 0 || phase_length == 0)
    {
        throw std::invalid_argument("invalid window parameters");
    }

    std::mt19937 rng(seed);
    std::uniform_int_distribution<int> position(0, static_cast<int>(width - 1));
    const std::size_t step = std::max<std::size_t>(1, width / 4);

    std::vector<int> out(count);

    for(std::size_t i = 0; i < count; ++i)
    {
        out[i] = 1 + static_cast<int>((i / phase_length) * step) + position(rng);
    }
    return out;
}

// Вероятность ключа ранга r пропорциональна 1 / r^exponent.
inline std::vector<int> zipf(std::size_t count, std::size_t width, double exponent, std::uint32_t seed)
{
    check_range(count, width);

    if(width == 0 || !std::isfinite(exponent) || exponent <= 0)
    {
        throw std::invalid_argument("invalid Zipf parameters");
    }

    std::vector<double> weights(width);

    for(std::size_t i = 0; i < width; ++i)
    {
        weights[i] = 1.0 / std::pow(static_cast<double>(i + 1), exponent);
    }

    std::mt19937 rng(seed);
    std::discrete_distribution<int> rank(weights.begin(), weights.end());

    std::vector<int> out(count);

    for(int& key : out)
    {
        key = rank(rng) + 1;
    }
    return out;
}

// В первой половине A получает 90% запросов, во второй популярность переходит к B.
inline std::vector<int> popularity_flip(std::size_t count, std::size_t width, std::uint32_t seed)
{
    check_range(count, width);

    if(2 * width > MAX_PAGE_KEY)
    {
        throw std::invalid_argument("Слишком широкие фазы");
    }

    if(width == 0)
    {
        throw std::invalid_argument("flip width must be positive");
    }

    std::mt19937 rng(seed);
    std::uniform_int_distribution<int> percent(0, 99);
    std::uniform_int_distribution<int> position(1, static_cast<int>(width));

    std::vector<int> out(count);

    for(std::size_t i = 0; i < count; ++i)
    {
        bool group_b = percent(rng) >= 90;
        if(i >= count / 2)
        {
            group_b = !group_b;
        }
        out[i] = position(rng) + (group_b ? static_cast<int>(width) : 0);
    }
    return out;
}

// Пример модели двух масштабов локальности: горячие 80%, тёплые 15%, холодные 5%.
inline std::vector<int> two_scales(std::size_t count, std::size_t hot, std::size_t warm, std::size_t cold,
                                   std::uint32_t seed)
{
    check_range(count, hot);
    check_range(count, warm);
    check_range(count, cold);

    if(hot + warm + cold > MAX_PAGE_KEY)
    {
        throw std::invalid_argument("Слишком большие множества");
    }

    if(hot == 0 || warm == 0 || cold == 0)
    {
        throw std::invalid_argument("invalid locality sizes");
    }
    
    std::mt19937 rng(seed);
    std::uniform_int_distribution<int> percent(0, 99);
    std::uniform_int_distribution<int> h(1, static_cast<int>(hot));
    std::uniform_int_distribution<int> w(static_cast<int>(hot + 1), static_cast<int>(hot + warm));
    std::uniform_int_distribution<int> c(static_cast<int>(hot + warm + 1), static_cast<int>(hot + warm + cold));

    std::vector<int> out(count);

    for(int& key : out)
    {
        const int p = percent(rng);
        key = p < 80 ? h(rng) : (p < 95 ? w(rng) : c(rng));
    }
    return out;
}

// Создаёт набор нагрузок по вместимостям уровней; сами кеши не изменяет.
inline std::vector<Pattern> make_patterns(std::size_t count, std::size_t l1_capacity,
                                          std::size_t last_capacity, std::size_t total_capacity,
                                          std::uint32_t seed = 42)
{
    if(count > MAX_REQUESTS || l1_capacity > MAX_PAGE_KEY || last_capacity > MAX_PAGE_KEY ||
       total_capacity < std::max(l1_capacity, last_capacity))
    {
        throw std::invalid_argument("invalid pattern suite parameters");
    }

    const auto l1_width = std::clamp<std::size_t>(l1_capacity, 2, MAX_PAGE_KEY / 16);
    const auto last_width = std::clamp<std::size_t>(last_capacity, 2, MAX_PAGE_KEY / 16);
    const auto total_width = std::clamp<std::size_t>(total_capacity, 2, MAX_PAGE_KEY / 16);

    std::vector<Pattern> patterns;

    auto add = [&](std::string name, std::vector<int> keys, bool randomized = false)
    {
        for(const auto& pattern : patterns)
        {
            if(pattern.name == name)
            {
                return;
            }
        }
        if(keys.size() != count)
        {
            throw std::logic_error("wrong generated request count");
        }
        for(int key : keys)
        {
            if(key < 1 || key > 1000000)
            {
                throw std::out_of_range("generated key outside current big_data");
            }
        }
        patterns.push_back({std::move(name), std::move(keys), randomized,
                            randomized ? std::optional<std::uint32_t>(seed) : std::nullopt});
    };

    add("one_key", std::vector<int>(count, 1));
    add("unique_scan", cycle(count, std::max<std::size_t>(1, count)));
    add("uniform_w10000", uniform(count, 10000, seed), true);

    std::vector<std::size_t> widths{l1_width / 2,    l1_width - 1,    l1_width,
                                    l1_width + 1,    last_width - 1,  last_width,
                                    last_width + 1,  total_width - 1, total_width,
                                    total_width + 1, 2 * total_width};
    std::sort(widths.begin(), widths.end());

    auto new_end = std::unique(widths.begin(), widths.end());
    widths.erase(new_end, widths.end());

    for(std::size_t width : widths)
    {
        const std::string suffix = "_w" + std::to_string(width);

        add("cycle" + suffix, cycle(count, width));
        add("uniform" + suffix, uniform(count, width, seed), true);
        add("shuffled_cycles" + suffix, shuffled_cycles(count, width, seed), true);
    }

    for(std::size_t width : {l1_width + 1, last_width + 1, total_width + 1})
    {
        add("ping_pong_w" + std::to_string(width), ping_pong(count, width));
    }

    for(std::size_t burst : {2U, 8U, 32U})
    {
        add("burst_r" + std::to_string(burst), bursts(count, burst));
    }

    for(std::size_t width : {l1_width - 1, l1_width + 1, last_width + 1, total_width + 1})
    {
        add("delayed_pairs_w" + std::to_string(width), delayed_pairs(count, width));
    }

    for(std::size_t hot : {std::max<std::size_t>(1, l1_width / 2), l1_width + 1})
    {
        for(int p : {50, 80, 95})
        {
            add("hot_cold_h" + std::to_string(hot) + "_p" + std::to_string(p),
                hot_cold(count, hot, 4 * total_width, p, seed), true);
        }
        for(std::size_t scan : {l1_width, last_width, 2 * total_width})
        {
            add("hot_scan_h" + std::to_string(hot) + "_s" + std::to_string(scan), hot_scan(count, hot, scan));
        }
    }

    for(std::size_t width : {l1_width, last_width})
    {
        for(std::size_t phase_length : {2 * width, 16 * width})
        {
            const std::string suffix = "_w" + std::to_string(width) + "_len" + std::to_string(phase_length);
            add("new_phases" + suffix, phases(count, width, phase_length, false));
            add("return_phases" + suffix, phases(count, width, phase_length, true));
            add("moving_window" + suffix, moving_window(count, width, phase_length, seed), true);
        }
        add("popularity_flip_w" + std::to_string(width), popularity_flip(count, width, seed), true);
    }

    for(int exponent_tenths : {6, 10, 14})
    {
        add("zipf_s" + std::to_string(exponent_tenths) + "_w" + std::to_string(4 * total_width),
            zipf(count, 4 * total_width, exponent_tenths / 10.0, seed), true);
    }

    add("two_scales_80_15_5",
        two_scales(count, std::max<std::size_t>(1, l1_width / 2), last_width, 4 * total_width, seed),
        true);

    // У этих двух потоков ТОЧНО одинаковые количества обращений к каждому ключу.
    // Отличается только порядок. Это отделяет локальность от частотного распределения.
    auto grouped = bursts(count, 32);
    for(int& key : grouped)
    {
        key = 1 + (key - 1) % static_cast<int>(2 * total_width);
    }
    add("same_histogram_grouped", grouped);

    std::mt19937 rng(seed);
    std::shuffle(grouped.begin(), grouped.end(), rng);

    add("same_histogram_shuffled", std::move(grouped), true);

    return patterns;
}

struct Settings
{
    std::size_t count = 100;
    std::size_t width = 4;
    std::size_t hot = 2;
    std::size_t cold = 16;
    int hot_percent = 90;
    std::size_t scan_length = 16;
    std::size_t phase_length = 32;
    std::size_t burst = 4;
    double exponent = 1.0;
    std::uint32_t seed = 42;
};

inline Pattern generate(const std::string& name, const Settings& settings)
{
    const auto& p = settings;

    check_range(p.count, p.width);
    Pattern result;
    result.name = name;
    std::ostringstream exponent;
    exponent << std::setprecision(std::numeric_limits<double>::max_digits10) << p.exponent;


    if(name == "one_key")
    {
        result.keys.assign(p.count, 1);
    }
    else if(name == "unique_scan")
    {
        result.keys = cycle(p.count, std::max<std::size_t>(1, p.count));
    }
    else if(name == "cycle")
    {
        result.keys = cycle(p.count, p.width);
    }
    else if(name == "bursts")
    {
        result.keys = bursts(p.count, p.burst);
    }
    else if(name == "delayed_pairs")
    {
        result.keys = delayed_pairs(p.count, p.width);
    }
    else if(name == "uniform")
    {
        result.keys = uniform(p.count, p.width, p.seed);
        result.randomized = true;
    }
    else if(name == "hot_cold")
    {
        result.keys = hot_cold(p.count, p.hot, p.cold, p.hot_percent, p.seed);
        result.randomized = true;
    }
    else if(name == "hot_scan")
    {
        result.keys = hot_scan(p.count, p.hot, p.scan_length);
    }
    else if(name == "phases")
    {
        result.keys = phases(p.count, p.width, p.phase_length, false);
    }
    else if(name == "return_phases")
    {
        result.keys = phases(p.count, p.width, p.phase_length, true);
    }
    else if(name == "moving_window")
    {
        result.keys = moving_window(p.count, p.width, p.phase_length, p.seed);
        result.randomized = true;
    }
    else if(name == "zipf")
    {
        result.keys = zipf(p.count, p.width, p.exponent, p.seed);
        result.randomized = true;
    }
    else if(name == "grouped" || name == "interleaved")
    {
        result.keys = cycle(p.count, p.width);
        if(name == "grouped")
        {
            std::sort(result.keys.begin(), result.keys.end());
        }
    }
    else
    {
        throw std::invalid_argument("Неизвестное семейство: " + name);
    }
    if(result.randomized)
    {
        result.seed = p.seed;
    }
    return result;
}

}

#endif
