// LargeNumberCSV.cpp : This file contains the 'main' function. Program execution begins and ends there.
//

#include <iostream>
#include <boost/beast/core.hpp>
#include <boost/beast/websocket.hpp>
#include <boost/asio/strand.hpp>
#include <cstdlib>
#include <functional>
#include <iostream>
#include <memory>
#include <string>
#include <chrono>
#include <thread>

#define _WIN32_WINNT 0x0601

namespace beast = boost::beast;         // from <boost/beast.hpp>
namespace http = beast::http;           // from <boost/beast/http.hpp>
namespace websocket = beast::websocket; // from <boost/beast/websocket.hpp>
namespace net = boost::asio;            // from <boost/asio.hpp>
using tcp = boost::asio::ip::tcp;       // from <boost/asio/ip/tcp.hpp>

//------------------------------------------------------------------------------

// Report a failure
void
fail(beast::error_code ec, char const* what)
{
    std::cerr << what << ": " << ec.message() << "\n";
}

// Sends a WebSocket message and prints the response
class session : public std::enable_shared_from_this<session>
{
    tcp::resolver resolver_;
    websocket::stream<beast::tcp_stream> ws_;
    beast::flat_buffer buffer_;
    std::string host_;
    std::string text_;

public:
    // Resolver and socket require an io_context
    explicit
        session(net::io_context& ioc)
        : resolver_(net::make_strand(ioc))
        , ws_(net::make_strand(ioc))
    {
    }

    // Start the asynchronous operation
    void
        run(
            char const* host,
            char const* port,
            char const* text)
    {
        // Save these for later
        host_ = host;
        text_ = text;

        // Look up the domain name
        resolver_.async_resolve(
            host,
            port,
            beast::bind_front_handler(
                &session::on_resolve,
                shared_from_this()));
    }

    void
        on_resolve(
            beast::error_code ec,
            tcp::resolver::results_type results)
    {
        if (ec)
            return fail(ec, "resolve");

        // Set the timeout for the operation
        beast::get_lowest_layer(ws_).expires_after(std::chrono::seconds(30));

        // Make the connection on the IP address we get from a lookup
        beast::get_lowest_layer(ws_).async_connect(
            results,
            beast::bind_front_handler(
                &session::on_connect,
                shared_from_this()));
    }

    void
        on_connect(beast::error_code ec, tcp::resolver::results_type::endpoint_type ep)
    {
        if (ec)
            return fail(ec, "connect");

        // Turn off the timeout on the tcp_stream, because
        // the websocket stream has its own timeout system.
        beast::get_lowest_layer(ws_).expires_never();

        // Set suggested timeout settings for the websocket
        ws_.set_option(
            websocket::stream_base::timeout::suggested(
                beast::role_type::client));

        // Set a decorator to change the User-Agent of the handshake
        ws_.set_option(websocket::stream_base::decorator(
            [](websocket::request_type& req)
            {
                req.set(http::field::user_agent,
                    std::string(BOOST_BEAST_VERSION_STRING) +
                    " websocket-client-async");
            }));

        // Update the host_ string. This will provide the value of the
        // Host HTTP header during the WebSocket handshake.
        // See https://tools.ietf.org/html/rfc7230#section-5.4
        host_ += ':' + std::to_string(ep.port());

        // Perform the websocket handshake
        ws_.async_handshake(host_, "/",
            beast::bind_front_handler(
                &session::on_handshake,
                shared_from_this()));
    }

    void
        on_handshake(beast::error_code ec)
    {
        if (ec)
            return fail(ec, "handshake");

        // Send the message
        ws_.async_write(
            net::buffer(text_),
            beast::bind_front_handler(
                &session::on_write,
                shared_from_this()));
    }

    void
        on_write(
            beast::error_code ec,
            std::size_t bytes_transferred)
    {
        boost::ignore_unused(bytes_transferred);

        if (ec)
            return fail(ec, "write");

        // Read a message into our buffer
        ws_.async_read(
            buffer_,
            beast::bind_front_handler(
                &session::on_read,
                shared_from_this()));
    }

    void
        on_read(
            beast::error_code ec,
            std::size_t bytes_transferred)
    {
        boost::ignore_unused(bytes_transferred);

        if (ec)
            return fail(ec, "read");

        
        std::cout << "\nWill send again " << std::endl;

        std::this_thread::sleep_for(std::chrono::milliseconds(1000));

        // Send the message
        ws_.async_write(
            net::buffer(text_),
            beast::bind_front_handler(
                &session::on_write,
                shared_from_this()));


        /*// Close the WebSocket connection
        ws_.async_close(websocket::close_code::normal,
            beast::bind_front_handler(
                &session::on_close,
                shared_from_this()));*/
    }

    void
        on_close(beast::error_code ec)
    {
        if (ec)
            return fail(ec, "close");

        // If we get here then the connection is closed gracefully

        // The make_printable() function helps print a ConstBufferSequence
        std::cout << beast::make_printable(buffer_.data()) << std::endl;
    }
};

//------------------------------------------------------------------------------

//Retrieving Data from Cells
/*void RetrievingDataFromCells()
{
    //Source directory path
    StringPtr dirPath = new String("..\\Data\\");

    //Path of input excel file
    StringPtr sampleData = dirPath->StringAppend(new String("sampleData.csv"));

    //Read input excel file
    intrusive_ptr<IWorkbook> workbook = Factory::CreateIWorkbook(sampleData);

    //Accessing the third worksheet in the Excel file
    intrusive_ptr<IWorksheet> worksheet = workbook->GetIWorksheets()->GetObjectByIndex(2);

    //Get cells from sheet
    intrusive_ptr<ICells> cells = worksheet->GetICells();

    //Variable declarations
    intrusive_ptr<String> strVal;
    intrusive_ptr<Aspose::Cells::Systems::DateTime> dateVal;
    Aspose::Cells::Systems::Double dblVal;
    Aspose::Cells::Systems::Boolean boolVal;

    std::cout << "\n Reading file \n";

    for (int i = 0; i < cells->GetCount(); i++)
    {
        intrusive_ptr<ICell> cell = cells->GetObjectByIndex(i);

        switch (cell->GetType())
        {
            //Evaluating the data type of the cell data for string value
        case CellValueType_IsString:
            Console::WriteLine(new String("Cell Value Type Is String."));
            strVal = cell->GetStringValue();
            break;
            //Evaluating the data type of the cell data for double value
        case CellValueType_IsNumeric:
            Console::WriteLine(new String("Cell Value Type Is Numeric."));
            dblVal = cell->GetDoubleValue();
            break;
            //Evaluating the data type of the cell data for boolean value
        case CellValueType_IsBool:
            Console::WriteLine(new String("Cell Value Type Is Bool."));
            boolVal = cell->GetBoolValue();
            break;
            //Evaluating the data type of the cell data for date/time value
        case CellValueType_IsDateTime:
            Console::WriteLine(new String("Cell Value Type Is DateTime."));
            dateVal = cell->GetDateTimeValue();
            break;
            //Evaluating the unknown data type of the cell data
        case CellValueType_IsUnknown:
            cell->GetStringValue();
            break;
        default:
            break;
        }
    }
}*/
//-------------------------------------------------------------------------------------------

int main(int argc, char** argv)
{

    std::cout << "argc: " << argc << std::endl;
    std::cout << "0: " << argv[0] << std::endl;
    std::cout << "1: " << argv[1] << std::endl;
    std::cout << "2: " << argv[2] << std::endl;
    std::cout << "3: " << argv[3] << std::endl;

    //RetrievingDataFromCells();

    return 0;

    // Check command line arguments.
    if (argc != 4)
    {
        std::cerr <<
            "Usage: websocket-client-async <host> <port> <text>\n" <<
            "Example:\n" <<
            "    websocket-client-async echo.websocket.org 80 \"Hello, world!\"\n";
        return EXIT_FAILURE;
    }
    auto const host = argv[1];
    auto const port = argv[2];
    auto const text = argv[3];

    // The io_context is required for all I/O
    net::io_context ioc;

    // Launch the asynchronous operation
    std::make_shared<session>(ioc)->run(host, port, text);

    // Run the I/O service. The call will return when
    // the socket is closed.
    ioc.run();

    return EXIT_SUCCESS;
}

// Run program: Ctrl + F5 or Debug > Start Without Debugging menu
// Debug program: F5 or Debug > Start Debugging menu

// Tips for Getting Started: 
//   1. Use the Solution Explorer window to add/manage files
//   2. Use the Team Explorer window to connect to source control
//   3. Use the Output window to see build output and other messages
//   4. Use the Error List window to view errors
//   5. Go to Project > Add New Item to create new code files, or Project > Add Existing Item to add existing code files to the project
//   6. In the future, to open this project again, go to File > Open > Project and select the .sln file
