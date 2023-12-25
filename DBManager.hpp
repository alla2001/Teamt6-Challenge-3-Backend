#pragma once
#include "Logger.h"
#include "string"

using namespace std;

class User {
public :
	string m_firstname;
	string m_lastname;
	string m_email;
	User(string firstname, string lastname, string email)  { 
		m_firstname = string(firstname);
		m_lastname = string(lastname);
		m_email = string(email);
	}
	User() {
		
	}

};

class DBManager
{



public:
	string m_port;
	string m_ip;
	string m_username;
	string m_password;
	
	int RegisterUser(User user);
	int DistrubuteEmail(){
		boost::mysql::results result;
		string s = "SELECT * FROM clients;";

		LOGSQL(s)
			conn.execute(s, result); 
		//send emails
			return 0;
	};
	int ConnectToDB();
	
	DBManager( const  string& ip, const string& port, const  string& username, const  string& password) : m_port(port),  m_ip(ip) , m_password(password), m_username(username) { LOG("Manager Init") }
private :
	
	boost::asio::io_context ctx;
	boost::asio::ssl::context ssl_ctx = boost::asio::ssl::context(boost::asio::ssl::context::tls_client);
	boost::mysql::tcp_ssl_connection conn = boost::mysql::tcp_ssl_connection(ctx.get_executor(), ssl_ctx);
};

 int DBManager::RegisterUser(User user)
{
	 DistrubuteEmail();
	 boost::mysql::results result;
	 string s = "INSERT INTO clients (FirstName, LastName, Email) VALUES(\'"+user.m_firstname +"\', \'"+user.m_lastname +"\',  \'"+user.m_email+"\');";

	 LOGSQL(s)
	 conn.execute(s, result);
	 LOGSQL(result.info())
	return 0;
}

 int DBManager::ConnectToDB() {
	 // The execution context, required to run I/O operations.
	 

	 // The SSL context, required to establish TLS connections.
	 // The default SSL options are good enough for us at this point.


	 // Represents a connection to the MySQL server.
	

	 boost::asio::ip::tcp::resolver resolver(ctx.get_executor());

	 
	 auto endpoints = resolver.resolve(m_ip, m_port);

	 // The username and password to use
	 boost::mysql::handshake_params params(
		 m_username,                // username
			m_password,                // password
		 "client"  // database
	 );
	 LOG("Connecting to DB ...")
		 // Connect to the server using the first endpoint returned by the resolver
	 conn.connect(*endpoints.begin(), params);
	 
	 LOG("Connected To DB")
		 return 0;
 }


