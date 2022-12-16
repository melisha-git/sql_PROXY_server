#include <boost/asio.hpp>
#include <iostream>

#include <boost/asio.hpp>
#include <iostream>
#include <fstream>

class server {
private:
	static boost::asio::io_service io_service;
	static boost::asio::ip::tcp::acceptor acceptor;
	static boost::asio::ip::tcp::resolver resolver;
	static boost::asio::ip::tcp::resolver::iterator dst_iterator;
public:
	server();
	void start();
private:

	server& operator=(const server &) = delete;

	server(const server &) {}

	static void logging(const std::shared_ptr<std::vector<char>>&, int n);

	static void accept();

	static void reder_writer(std::shared_ptr<boost::asio::ip::tcp::socket>, 
		std::shared_ptr<boost::asio::ip::tcp::socket>, 
		std::shared_ptr<std::vector<char>> buffer, const bool &);
};

boost::asio::io_service server::io_service;
boost::asio::ip::tcp::acceptor server::acceptor{ io_service, boost::asio::ip::tcp::endpoint(boost::asio::ip::address_v4::loopback(), 5431) };
boost::asio::ip::tcp::resolver server::resolver(io_service);
boost::asio::ip::tcp::resolver::iterator server::dst_iterator = resolver.resolve(boost::asio::ip::tcp::resolver::query("127.0.0.1", "5432"));

server::server() {

}

void server::start() {
	accept();
	server::io_service.run();
}

void server::accept() {
	std::shared_ptr<boost::asio::ip::tcp::socket> src = std::make_shared<boost::asio::ip::tcp::socket>(io_service);
	// Ожидает принятия клиентов, при принятии вызывается функция - handler
	acceptor.async_accept(*src, [src](auto error) {
		std::cout << "accept " << error << std::endl;
		if (error)
			return;
		std::shared_ptr<boost::asio::ip::tcp::socket> dst = std::make_shared<boost::asio::ip::tcp::socket>(io_service);
		// принятие соединения
		dst->async_connect(*dst_iterator, [src, dst](auto error) {
			std::cout << "connect " << error << std::endl;
			if (error)
				return;
			reder_writer(src, dst, std::make_shared<std::vector<char>>(4096), true);
			reder_writer(dst, src, std::make_shared<std::vector<char>>(4096), false);
			});
		accept();
		});
}

void server::logging(const std::shared_ptr<std::vector<char>> &buffer, int n) {
	static int count = 0;

	std::string line((buffer->data()), n - 1);
	if (line.empty())
		return;
	if (static_cast<int>(line[0]) != 81)
		return;
	std::ofstream file("log.txt", std::ios::app |  std::ios::binary);
	if (!file.is_open()) {
		std::cerr << "Log.txt not open\n";
		return;
	}
	//file << static_cast<int>(line[0]) << " " << static_cast<int>(line[1]) << " " << static_cast<int>(line[2]) << " " << static_cast<int>(line[4]) << " " << static_cast<int>(line[5]) << std::endl;
	line = line.substr(5, n - 6);
	file << line << std::endl;
	file << std::endl;
	file.close();
}

void server::reder_writer(std::shared_ptr<boost::asio::ip::tcp::socket> src,
					std::shared_ptr<boost::asio::ip::tcp::socket> dst,
					std::shared_ptr<std::vector<char>> buffer, 
					const bool &is_request) {
	// асинхронное чтение, после прочтения вызывается обработчик
	src->async_read_some(boost::asio::buffer(*buffer), [src, dst, buffer, is_request](auto error, auto n) {
		//std::cout << "read " << n << ' ' << error << std::endl;
		if (error) {
			src->close();
			dst->close();
			return;
		}
		if (is_request)
			logging(buffer, n);
		async_write(*dst, boost::asio::buffer(buffer->data(), n), boost::asio::transfer_all(),
			[src, dst, buffer, is_request](auto error, auto n) {
				//std::cout << "write " << n << ' ' << error << std::endl;
				if (error) {
					src->close();
					dst->close();
					return;
				}
				reder_writer(src, dst, buffer, is_request);
			});
		});
}

int main() {
	try {
		server s;
		s.start();
	}
	catch (std::exception& e) {
		std::cerr << "excpt main " << e.what() << std::endl;
	}
}