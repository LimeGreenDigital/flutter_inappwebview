#include "../in_app_browser/in_app_browser.h"
#include "../types/base_callback_result.h"
#include "../types/content_world.h"
#include "../utils/flutter.h"
#include "../utils/log.h"
#include "../utils/strconv.h"
#include "../utils/string.h"
#include "in_app_webview.h"
#include "webview_channel_delegate.h"

namespace flutter_inappwebview_plugin
{
  WebViewChannelDelegate::WebViewChannelDelegate(InAppWebView* webView, flutter::BinaryMessenger* messenger)
    : webView(webView), ChannelDelegate(messenger, InAppWebView::METHOD_CHANNEL_NAME_PREFIX + variant_to_string(webView->id))
  {}

  WebViewChannelDelegate::WebViewChannelDelegate(InAppWebView* webView, flutter::BinaryMessenger* messenger, const std::string& name)
    : webView(webView), ChannelDelegate(messenger, name)
  {}

  WebViewChannelDelegate::ShouldOverrideUrlLoadingCallback::ShouldOverrideUrlLoadingCallback()
  {
    decodeResult = [](const flutter::EncodableValue* value)
      {
        if (!value || value->IsNull()) {
          return NavigationActionPolicy::cancel;
        }
        auto navigationPolicy = std::get<int>(*value);
        return static_cast<NavigationActionPolicy>(navigationPolicy);
      };
  }

  WebViewChannelDelegate::CallJsHandlerCallback::CallJsHandlerCallback()
  {
    decodeResult = [](const flutter::EncodableValue* value)
      {
        return value;
      };
  }

  void WebViewChannelDelegate::HandleMethodCall(const flutter::MethodCall<flutter::EncodableValue>& method_call,
    std::unique_ptr<flutter::MethodResult<flutter::EncodableValue>> result)
  {
    if (!webView) {
      result->Success();
      return;
    }

    auto& arguments = std::get<flutter::EncodableMap>(*method_call.arguments());
    auto& methodName = method_call.method_name();

    if (string_equals(methodName, "getUrl")) {
      result->Success(make_fl_value(webView->getUrl()));
    }
    else if (string_equals(methodName, "getTitle")) {
      result->Success(make_fl_value(webView->getUrl()));
    }
    else if (string_equals(methodName, "loadUrl")) {
      auto urlRequest = std::make_unique<URLRequest>(get_fl_map_value<flutter::EncodableMap>(arguments, "urlRequest"));
      webView->loadUrl(std::move(urlRequest));
      result->Success(true);
    }
    else if (string_equals(methodName, "loadFile")) {
      auto assetFilePath = get_fl_map_value<std::string>(arguments, "assetFilePath");
      webView->loadFile(assetFilePath);
      result->Success(true);
    }
    else if (string_equals(methodName, "loadData")) {
      auto data = get_fl_map_value<std::string>(arguments, "data");
      webView->loadData(data);
      result->Success(true);
    }
    else if (string_equals(methodName, "reload")) {
      webView->reload();
      result->Success(true);
    }
    else if (string_equals(methodName, "goBack")) {
      webView->goBack();
      result->Success(true);
    }
    else if (string_equals(methodName, "canGoBack")) {
      result->Success(webView->canGoBack());
    }
    else if (string_equals(methodName, "goForward")) {
      webView->goForward();
      result->Success(true);
    }
    else if (string_equals(methodName, "canGoForward")) {
      result->Success(webView->canGoForward());
    }
    else if (string_equals(methodName, "goBackOrForward")) {
      auto steps = get_fl_map_value<int>(arguments, "steps");
      webView->goBackOrForward(steps);
      result->Success(true);
    }
    else if (string_equals(methodName, "canGoBackOrForward")) {
      auto result_ = std::shared_ptr<flutter::MethodResult<flutter::EncodableValue>>(std::move(result));

      auto steps = get_fl_map_value<int>(arguments, "steps");
      webView->canGoBackOrForward(steps, [result_ = std::move(result_)](const bool& value)
        {
          result_->Success(value);
        });
    }
    else if (string_equals(methodName, "isLoading")) {
      result->Success(webView->isLoading());
    }
    else if (string_equals(methodName, "stopLoading")) {
      webView->stopLoading();
      result->Success(true);
    }
    else if (string_equals(methodName, "evaluateJavascript")) {
      auto result_ = std::shared_ptr<flutter::MethodResult<flutter::EncodableValue>>(std::move(result));

      auto source = get_fl_map_value<std::string>(arguments, "source");
      auto contentWorldMap = get_optional_fl_map_value<flutter::EncodableMap>(arguments, "contentWorld");
      std::shared_ptr<ContentWorld> contentWorld = contentWorldMap.has_value() ? std::make_shared<ContentWorld>(contentWorldMap.value()) : ContentWorld::page();
      webView->evaluateJavascript(source, std::move(contentWorld), [result_ = std::move(result_)](const std::string& value)
        {
          result_->Success(value);
        });
    }
    else if (string_equals(methodName, "callAsyncJavaScript")) {
      auto result_ = std::shared_ptr<flutter::MethodResult<flutter::EncodableValue>>(std::move(result));

      auto functionBody = get_fl_map_value<std::string>(arguments, "functionBody");
      auto argumentsAsJson = get_fl_map_value<std::string>(arguments, "arguments");
      auto contentWorldMap = get_optional_fl_map_value<flutter::EncodableMap>(arguments, "contentWorld");
      std::shared_ptr<ContentWorld> contentWorld = contentWorldMap.has_value() ? std::make_shared<ContentWorld>(contentWorldMap.value()) : ContentWorld::page();
      webView->callAsyncJavaScript(functionBody, argumentsAsJson, std::move(contentWorld), [result_ = std::move(result_)](const std::string& value)
        {
          result_->Success(value);
        });
    }
    else if (string_equals(methodName, "getCopyBackForwardList")) {
      auto result_ = std::shared_ptr<flutter::MethodResult<flutter::EncodableValue>>(std::move(result));
      webView->getCopyBackForwardList([result_ = std::move(result_)](const std::unique_ptr<WebHistory> value)
        {
          result_->Success(value->toEncodableMap());
        });
    }
    else if (string_equals(methodName, "addUserScript")) {
      auto userScript = std::make_unique<UserScript>(get_fl_map_value<flutter::EncodableMap>(arguments, "userScript"));
      webView->addUserScript(std::move(userScript));
      result->Success(true);
    }
    else if (string_equals(methodName, "removeUserScript")) {
      auto index = get_fl_map_value<int>(arguments, "index");
      auto userScript = std::make_unique<UserScript>(get_fl_map_value<flutter::EncodableMap>(arguments, "userScript"));
      webView->removeUserScript(index, std::move(userScript));
      result->Success(true);
    }
    else if (string_equals(methodName, "removeUserScriptsByGroupName")) {
      auto groupName = get_fl_map_value<std::string>(arguments, "groupName");
      webView->removeUserScriptsByGroupName(groupName);
      result->Success(true);
    }
    else if (string_equals(methodName, "removeAllUserScripts")) {
      webView->removeAllUserScripts();
      result->Success(true);
    }
    else if (string_equals(methodName, "takeScreenshot")) {
      auto result_ = std::shared_ptr<flutter::MethodResult<flutter::EncodableValue>>(std::move(result));
      auto screenshotConfigurationMap = get_optional_fl_map_value<flutter::EncodableMap>(arguments, "screenshotConfiguration");
      std::optional<std::unique_ptr<ScreenshotConfiguration>> screenshotConfiguration =
        screenshotConfigurationMap.has_value() ? std::make_unique<ScreenshotConfiguration>(screenshotConfigurationMap.value()) : std::optional<std::unique_ptr<ScreenshotConfiguration>>{};
      webView->takeScreenshot(std::move(screenshotConfiguration), [result_ = std::move(result_)](const std::optional<std::string> data)
        {
          result_->Success(make_fl_value(data));
        });
    }
    // for inAppBrowser
    else if (webView->inAppBrowser && string_equals(methodName, "show")) {
      webView->inAppBrowser->show();
      result->Success(true);
    }
    else if (webView->inAppBrowser && string_equals(methodName, "hide")) {
      webView->inAppBrowser->hide();
      result->Success(true);
    }
    else if (webView->inAppBrowser && string_equals(methodName, "close")) {
      webView->inAppBrowser->close();
      result->Success(true);
    }
    else {
      result->NotImplemented();
    }
  }

  void WebViewChannelDelegate::onLoadStart(const std::optional<std::string>& url) const
  {
    if (!channel) {
      return;
    }

    auto arguments = std::make_unique<flutter::EncodableValue>(flutter::EncodableMap{
      {"url", make_fl_value(url)},
      });
    channel->InvokeMethod("onLoadStart", std::move(arguments));
  }

  void WebViewChannelDelegate::onLoadStop(const std::optional<std::string>& url) const
  {
    if (!channel) {
      return;
    }

    auto arguments = std::make_unique<flutter::EncodableValue>(flutter::EncodableMap{
      {"url", make_fl_value(url)},
      });
    channel->InvokeMethod("onLoadStop", std::move(arguments));
  }

  void WebViewChannelDelegate::shouldOverrideUrlLoading(std::shared_ptr<NavigationAction> navigationAction, std::unique_ptr<ShouldOverrideUrlLoadingCallback> callback) const
  {
    if (!channel) {
      return;
    }

    auto arguments = std::make_unique<flutter::EncodableValue>(navigationAction->toEncodableMap());
    channel->InvokeMethod("shouldOverrideUrlLoading", std::move(arguments), std::move(callback));
  }

  void WebViewChannelDelegate::onReceivedError(std::shared_ptr<WebResourceRequest> request, std::shared_ptr<WebResourceError> error) const
  {
    if (!channel) {
      return;
    }

    auto arguments = std::make_unique<flutter::EncodableValue>(flutter::EncodableMap{
      {"request", request->toEncodableMap()},
      {"error", error->toEncodableMap()},
      });
    channel->InvokeMethod("onReceivedError", std::move(arguments));
  }

  void WebViewChannelDelegate::onReceivedHttpError(std::shared_ptr<WebResourceRequest> request, std::shared_ptr<WebResourceResponse> errorResponse) const
  {
    if (!channel) {
      return;
    }

    auto arguments = std::make_unique<flutter::EncodableValue>(flutter::EncodableMap{
      {"request", request->toEncodableMap()},
      {"errorResponse", errorResponse->toEncodableMap()},
      });
    channel->InvokeMethod("onReceivedHttpError", std::move(arguments));
  }

  void WebViewChannelDelegate::onTitleChanged(const std::optional<std::string>& title) const
  {
    if (!channel) {
      return;
    }

    auto arguments = std::make_unique<flutter::EncodableValue>(flutter::EncodableMap{
      {"title", make_fl_value(title)}
      });
    channel->InvokeMethod("onTitleChanged", std::move(arguments));

    if (webView && webView->inAppBrowser) {
      webView->inAppBrowser->didChangeTitle(title);
    }
  }

  void WebViewChannelDelegate::onUpdateVisitedHistory(const std::optional<std::string>& url, const std::optional<bool>& isReload) const
  {
    if (!channel) {
      return;
    }

    auto arguments = std::make_unique<flutter::EncodableValue>(flutter::EncodableMap{
      {"url", make_fl_value(url)},
      {"isReload", make_fl_value(isReload)}
      });
    channel->InvokeMethod("onUpdateVisitedHistory", std::move(arguments));
  }

  void WebViewChannelDelegate::onCallJsHandler(const std::string& handlerName, const std::string& args, std::unique_ptr<CallJsHandlerCallback> callback) const
  {
    if (!channel) {
      return;
    }

    auto arguments = std::make_unique<flutter::EncodableValue>(flutter::EncodableMap{
      {"handlerName", handlerName},
      {"args", args}
      });
    channel->InvokeMethod("onCallJsHandler", std::move(arguments), std::move(callback));
  }

  void WebViewChannelDelegate::onConsoleMessage(const std::string& message, const int64_t& messageLevel) const
  {
    if (!channel) {
      return;
    }

    auto arguments = std::make_unique<flutter::EncodableValue>(flutter::EncodableMap{
      {"message", message},
      {"messageLevel", messageLevel}
      });
    channel->InvokeMethod("onConsoleMessage", std::move(arguments));
  }

  WebViewChannelDelegate::~WebViewChannelDelegate()
  {
    debugLog("dealloc WebViewChannelDelegate");
    webView = nullptr;
  }
}