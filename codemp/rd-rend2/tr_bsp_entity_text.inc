// The BSP provides a byte span, which need not include a terminal NUL.
static char* R_CopyBSPEntityText(const byte* source, int length) {
    if (length < 0 || length == INT_MAX) return nullptr;
    auto* text = static_cast<char*>(ri.Hunk_Alloc(length + 1, h_low));
    if (length) memcpy(text, source, length);
    text[length] = '\0';
    return text;
}
