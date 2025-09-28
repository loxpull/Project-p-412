#include<iostream>
#include <string>
#include <vector>
#include <boost/asio.hpp>
#include <boost/beast.hpp>
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/xml_parser.hpp>

namespace asio = boost::asio;
namespace beast = boost::beast;
namespace http = beast::http;
namespace pt = boost::property_tree;
namespace xml = boost::property_tree::xml_parser;
using tcp = asio::ip::tcp;


// Структура для хранения данных о валюте
struct CurrencyRate {
    std::string code;
    std::string name;
    double value;
    int nominal;
};

// Функция для получения XML данных
std::string get_xml_data() {
    try {
        asio::io_context io_context;
        tcp::resolver resolver(io_context);
        auto const results = resolver.resolve("www.cbr.ru", "80");
        tcp::socket socket(io_context);
        asio::connect(socket, results);

        // Запрос для получения ежедневных курсов в XML формате
        http::request<http::string_body> req{
            http::verb::get,
            "/scripts/XML_daily.asp?date_req=",
            11
        };
        req.set(http::field::host, "www.cbr.ru");
        req.set(http::field::user_agent, "Boost Beast Client");
        req.set(http::field::accept, "application/xml");

        http::write(socket, req);

        beast::flat_buffer buffer;
        http::response<http::dynamic_body> res;
        http::read(socket, buffer, res);

        beast::error_code ec;
        socket.shutdown(tcp::socket::shutdown_both, ec);

        return boost::beast::buffers_to_string(res.body().data());

    }
    catch (const std::exception& e) {
        std::cerr << "Ошибка получения данных: " << e.what() << std::endl;
        return "";
    }
}

// Функция для парсинга XML и извлечения курсов валют
std::vector<CurrencyRate> parse_currency_rates(const std::string& xml_data) {
    std::vector<CurrencyRate> rates;

    try {
        std::istringstream iss(xml_data);
        pt::ptree ptree;

        // Читаем XML
        pt::read_xml(iss, ptree);

        // Получаем дату
        std::string date = ptree.get<std::string>("ValCurs.<xmlattr>.Date");
        std::cout << "Курсы валют ЦБ РФ на " << date << std::endl;
        std::cout << "==============================================" << std::endl;

        // Извлекаем данные о валютах
        for (const auto& valute : ptree.get_child("ValCurs")) {
            if (valute.first == "Valute") {
                CurrencyRate rate;
                rate.code = valute.second.get<std::string>("<xmlattr>.ID");
                rate.name = valute.second.get<std::string>("Name");

                // Заменяем запятую на точку для корректного преобразования в double
                std::string value_str = valute.second.get<std::string>("Value");
                std::replace(value_str.begin(), value_str.end(), ',', '.');
                rate.value = std::stod(value_str);

                rate.nominal = valute.second.get<int>("Nominal");
                rates.push_back(rate);
            }
        }

    }
    catch (const std::exception& e) {
        std::cerr << "Ошибка парсинга XML: " << e.what() << std::endl;
    }

    return rates;
}

// Функция для вывода курсов валют
void print_currency_rates(const std::vector<CurrencyRate>& rates) {
    for (const auto& rate : rates) {
        std::cout << rate.nominal << "\t " << rate.name << " (" << rate.code << "): "
            << rate.value << " руб." << std::endl;
    }
}

// Функция для поиска конкретной валюты по коду
auto find_currency_rate(const std::vector<CurrencyRate>& rates, const std::string& code) {
    for (const auto& rate : rates) {
        if (rate.code == code) {
            return rate.value / rate.nominal;
        }
    }
    return -1.0;
}

// Функция для получения курса конкретной валюты
double get_specific_currency_xml(const std::string& currency_code) {
    std::string xml_data = get_xml_data();
    if (xml_data.empty()) {
        return -1.0;
    }

    auto rates = parse_currency_rates(xml_data);
    return find_currency_rate(rates, currency_code);
}
// Функция для вывода курса конкретной валюты
auto print_currency_rate_by_code(const std::vector<CurrencyRate>& rates, const std::string& currency_code) {
    for (const auto& rate : rates) {
        if (rate.code == currency_code) {
            std::cout << "\nНайденная валюта:" << std::endl;
            std::cout << rate.nominal << " " << rate.name << " (" << rate.code << "): "
                << rate.value << " руб." << std::endl;
            std::cout << "Курс за 1 единицу: " << rate.value / rate.nominal << " руб." << std::endl;
            return;
        }
    }
    std::cout << "Валюта с кодом '" << currency_code << "' не найдена." << std::endl;
}

int main()
{
    
    setlocale(LC_ALL, "ru");
    std::cout << "Актуальные курсы валют от ЦБ РФ" << std::endl;
    std::cout << "Получение курсов валют ЦБ РФ в XML формате..." << std::endl;

    // Получаем XML данные
    std::string xml_data = get_xml_data();

    //if (!xml_data.empty()) {
    //    // Парсим и выводим все курсы
    //    auto rates = parse_currency_rates(xml_data);
    //    print_currency_rates(rates);

    //    std::cout << "\n==============================================" << std::endl;

    //    // Пример поиска конкретных валют
    //    std::vector<std::string> popular_currencies = {
    //        "R01235", // USD
    //        "R01239", // EUR
    //        "R01375"  // CNY
    //    };

    //    std::cout << "\nПопулярные валюты:" << std::endl;
    //    for (const auto& code : popular_currencies) {
    //        double rate = find_currency_rate(rates, code);
    //        if (rate > 0) {
    //            // Находим название валюты по коду
    //            for (const auto& r : rates) {
    //                if (r.code == code) {
    //                    std::cout << "1 " << r.name << ": " << rate << " руб." << std::endl;
    //                    break;
    //                }
    //            }
    //        }
    //    }
    //}


    std::cout << "\n==============================================" << std::endl;
    /*std::cout << "Получение отдельных курсов:" << std::endl;

    double usd_rate = get_specific_currency_xml("R01235");
    if (usd_rate > 0) {
        std::cout << "USD: " << usd_rate << " руб." << std::endl;
    }

    double eur_rate = get_specific_currency_xml("R01239");
    if (eur_rate > 0) {
        std::cout << "EUR: " << eur_rate << " руб." << std::endl;
    }*/


    int c;
    auto rates = parse_currency_rates(xml_data);
    

    while (true) {
        std::cout << "Выберите действие: " << "\n";
        std::cout << "1 - Получение курсов валют" << "\n";
        std::cout << "2 - Получение курса отдельных валют по коду" << "\n";
        std::cout << "3 - Расчет валюты" << "\n";
        std::cout << "4 - Выход" << "\n";

        std::cin >> c;

        switch (c) {
        case 1: {
            print_currency_rates(rates);
            std::cout << "\n==============================================" << std::endl;
            break;
        }
        case 2: {
            // код для получения курса по коду

            std::string currency_code;
            std::cout << "Введите код валюты: ";
            std::cin >> currency_code;
            print_currency_rate_by_code(rates, currency_code);
            break;
            
        }

        case 3: {
            // код для расчета валюты
            std::string currency_code1;
            std::string currency_code2;
            std::cout << "Введите код валюты, из которой вы хотите конвертировать: ";
            std::cin >> currency_code1;
            std::cout << "Введите код валюты, в которой вы хотите конвертировать: ";
            std::cin >> currency_code2;

            break;
        }
        case 4: {
            std::cout << "Выход из программы..." << std::endl;
            return 0;
        }
        default: {
            std::cout << "Неверный выбор! Попробуйте снова." << std::endl;
            break;
        }
        }

              // Небольшая пауза перед следующим выводом меню
              std::cout << "\nНажмите Enter для продолжения...";
              std::cin.ignore();
              std::cin.get();
        }

        return 0;
}

