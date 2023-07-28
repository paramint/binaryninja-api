#pragma once

#include <QtWidgets/QAbstractScrollArea>
#include <QtCore/QTimer>
#include "binaryninjaapi.h"
#include "viewframe.h"
#include "render.h"
#include "commentdialog.h"
#include "menus.h"
#include "uicontext.h"

/*!

	\defgroup tokenizedtextwidget TokenizedTextWidget
 	\ingroup uiapi
*/


/*!

    \ingroup tokenizedtextwidget
*/
enum BINARYNINJAUIAPI TokenizedTextWidgetSelectionStyle
{
	NoSelection,
	SelectLines,
	SelectOneToken,
	SelectTokens,
	SelectCharacters
};


/*!

    \ingroup tokenizedtextwidget
*/
struct BINARYNINJAUIAPI TokenizedTextWidgetCursorPosition
{
	size_t lineIndex = BN_INVALID_OPERAND;
	size_t tokenIndex = BN_INVALID_OPERAND;
	size_t characterIndex = BN_INVALID_OPERAND;

	// Directly from QMouseEvent, not used in comparator
	int cursorX;
	int cursorY;

	bool isValid() const { return lineIndex != BN_INVALID_OPERAND; }
	bool operator==(const TokenizedTextWidgetCursorPosition& other) const;
	bool operator!=(const TokenizedTextWidgetCursorPosition& other) const { return !(*this == other); }
	bool operator<(const TokenizedTextWidgetCursorPosition& other) const;
};

/*!
    QWidget that displays lines of InstructionTextTokens with the ability to make selections

    \ingroup tokenizedtextwidget
*/
class BINARYNINJAUIAPI TokenizedTextWidget :
    public QAbstractScrollArea
{
	Q_OBJECT

	UIActionHandler m_actionHandler;

	RenderContext m_render;
	int m_cols, m_rows;
	int m_wheelDelta;
	bool m_updatingScrollBar;

	TokenizedTextWidgetCursorPosition m_cursorPos, m_selectionStartPos, m_hoverPos;
	TokenizedTextWidgetSelectionStyle m_selectionMode;
	size_t m_hoverLine;
	bool m_cursorKeys;

	std::vector<BinaryNinja::LinearDisassemblyLine> m_lines;
	DisassemblySettingsRef m_settings;

	void adjustSize(int width, int height);
	void clampCursorPosition(TokenizedTextWidgetCursorPosition& pos);
	void clampSelectionToValid();

  private Q_SLOTS:
	void scrollBarMoved(int value);
	void scrollBarAction(int action);

  public:
	explicit TokenizedTextWidget(QWidget* parent,
	    const std::vector<BinaryNinja::LinearDisassemblyLine>& lines =
	        std::vector<BinaryNinja::LinearDisassemblyLine>());
	virtual ~TokenizedTextWidget();

	void bindActions();

	QFont font() const;
	void setFont(const QFont& font);

	DisassemblySettingsRef settings() { return m_settings; }
	const DisassemblySettingsRef& settings() const { return m_settings; }

	int getTopLine() const;
	int getVisibleColumns() const { return m_cols; }
	int getVisibleRows() const { return m_rows; }

	bool hasSelection() const;
	// Lines vs Tokens vs Characters vs NoSelection
	TokenizedTextWidgetSelectionStyle selectionStyle() const;
	// Lower bound of selection
	TokenizedTextWidgetCursorPosition selectionBegin() const;
	// Upper bound of selection
	TokenizedTextWidgetCursorPosition selectionEnd() const;
	// Originally highlighted selection base
	TokenizedTextWidgetCursorPosition selectionBase() const;
	// Position of cursor for movement operations
	TokenizedTextWidgetCursorPosition cursorPosition() const;

	void clearSelection();
	void setSelection(TokenizedTextWidgetCursorPosition base, TokenizedTextWidgetCursorPosition cursor);
	void setCursorPosition(TokenizedTextWidgetCursorPosition newPosition, bool selecting, bool cursorKeys, bool evenIfNoChange);
	void moveCursorHorizontal(int count, bool allTheWay, bool selecting, bool cursorKeys);
	void moveCursorVertical(int count, bool allTheWay, bool selecting, bool cursorKeys);

	HighlightTokenState highlightTokenState();
	UIActionHandler* actionHandler() { return &m_actionHandler; }
	virtual UIActionContext actionContext();

	void left(size_t count, bool selecting);
	void right(size_t count, bool selecting);
	void leftToWord(bool selecting);
	void rightToWord(bool selecting);
	void up(bool selecting);
	void down(bool selecting);
	void pageUp(bool selecting);
	void pageDown(bool selecting);
	void moveToStartOfLine(bool selecting);
	void moveToEndOfLine(bool selecting);
	void moveToStartOfView(bool selecting);
	void moveToEndOfView(bool selecting);

	void scrollLines(int count);
	void scrollLineToVisible(int lineIndex);
	void scrollLineToTop(int lineIndex);

	const std::vector<BinaryNinja::LinearDisassemblyLine>& getLines() const { return m_lines; }
	void setLines(const std::vector<BinaryNinja::LinearDisassemblyLine>& lines, bool resetScroll = true);
	void setLines(const std::vector<BinaryNinja::DisassemblyTextLine>& lines, bool resetScroll = true);
	void setLines(const std::vector<BinaryNinja::TypeDefinitionLine>& lines, bool resetScroll = true);

  Q_SIGNALS:
	void linesChanged();
	void selectionChanged(const TokenizedTextWidgetCursorPosition& begin, const TokenizedTextWidgetCursorPosition& end);
	void tokenLeftClicked(const TokenizedTextWidgetCursorPosition& position);
	void tokenRightClicked(const TokenizedTextWidgetCursorPosition& position);
	void tokenDoubleClicked(const TokenizedTextWidgetCursorPosition& position);
	void tokenOtherClicked(const TokenizedTextWidgetCursorPosition& position, Qt::MouseButton button);
	void tokenHovered(const TokenizedTextWidgetCursorPosition& position);
	void lineLeftClicked(size_t lineIndex);
	void lineRightClicked(size_t lineIndex);
	void lineDoubleClicked(size_t lineIndex);
	void lineOtherClicked(size_t lineIndex, Qt::MouseButton button);
	void lineHovered(size_t lineIndex);

  protected:
	virtual void resizeEvent(QResizeEvent* event) override;
	virtual void paintEvent(QPaintEvent* event) override;
	virtual void wheelEvent(QWheelEvent* event) override;
	virtual void mousePressEvent(QMouseEvent* event) override;
	virtual void mouseMoveEvent(QMouseEvent* event) override;
	virtual void mouseDoubleClickEvent(QMouseEvent* event) override;
	virtual void leaveEvent(QEvent* event) override;
};
