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
auto find_currency_rate(const std::vector<CurrencyRate>& rates, 
    const std::string& code) {
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
auto print_currency_rate_by_code(const std::vector<CurrencyRate>& rates, 
    const std::string& currency_code) {
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


// Функция для перевода из одной валюты в другую
double transfer_from_one_currency_to_another(const std::vector<CurrencyRate>& rates,
    const std::string& currency_code1,
    const std::string& currency_code2,
    double amount)
{
    // Находим курсы обеих валют
    double rate1 = -1.0;
    double rate2 = -1.0;
    std::string name1, name2;

    // Поиск первую валюту
    for (const auto& rate : rates) {
        if (rate.code == currency_code1) {
            rate1 = rate.value / rate.nominal;
            name1 = rate.name;
            break;
        }
    }

    // Поиск вторую валюту
    for (const auto& rate : rates) {
        if (rate.code == currency_code2) {
            rate2 = rate.value / rate.nominal;
            name2 = rate.name;
            break;
        }
    }

    // Проверяем, что обе валюты найдены
    if (rate1 <= 0 || rate2 <= 0) {
        std::cout << "Ошибка: не удалось найти курсы для указанных валют!" << std::endl;
        return -1.0;
    }


    // Проверяем, что сумма положительная
    if (amount <= 0) {
        std::cout << "Ошибка: сумма должна быть положительным числом!" << std::endl;
        return -1.0;
    }

    // Выполняем конвертацию по формуле: (amount * rate1) / rate2
    double result = (amount * rate1) / rate2;

    // Выводим детали конвертации
    std::cout << "\n--- Результат конвертации ---" << std::endl;
    std::cout << std::fixed << std::setprecision(2) << amount << " " << name1 << " = " << std::fixed << std::setprecision(2) << result << " " << name2 << std::endl;
    std::cout << "Курс: 1 " << name1 << " = " << (rate1 / rate2) << " " << name2 << std::endl;
    std::cout << "-----------------------------" << std::endl;

    return result;
}

int main()
{
    setlocale(LC_ALL, "ru");
    std::cout << "Актуальные курсы валют от ЦБ РФ" << std::endl;
    std::cout << "Получение курсов валют ЦБ РФ в XML формате..." << std::endl;

    // Получаем XML данные
    std::string xml_data = get_xml_data();

    std::cout << "\n==============================================" << std::endl;
  
    int key;
    auto rates = parse_currency_rates(xml_data);
    
    while (true) {
        std::cout << "Выберите действие: " << "\n";
        std::cout << "1 - Получение курсов валют" << "\n";
        std::cout << "2 - Получение курса отдельных валют по коду" << "\n";
        std::cout << "3 - Расчет валюты" << "\n";
        std::cout << "4 - Выход" << "\n";

        std::cin >> key;

        switch (key) {
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
            std::string currency_code1, currency_code2;
            double currency_count;
            std::string input;

            std::cout << "Введите код валюты, из которой вы хотите конвертировать: ";
            std::cin >> currency_code1;


            while (true) {
                std::cout << "Введите сумму, которую хотите перевести: ";
                std::cin >> input;

                try {
                    currency_count = std::stod(input);
                    if (currency_count > 0) {
                        break;
                    }
                    else {
                        std::cout << "Ошибка: сумма должна быть положительным числом! Попробуйте снова." << std::endl;
                    }
                }
                catch (const std::exception& e) {
                    std::cout << "Ошибка: введите числовое значение! Попробуйте снова." << std::endl;
                }
            }

            std::cout << "Введите код валюты, в которую вы хотите конвертировать: ";
            std::cin >> currency_code2;

            double result = transfer_from_one_currency_to_another(rates, currency_code1, currency_code2, currency_count);
            if (result >= 0) {
                std::cout << "Итоговая сумма: " << std::fixed << std::setprecision(2) << result << std::endl;
            }
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

