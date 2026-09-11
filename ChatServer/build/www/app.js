(function () {
    'use strict';

    // 服务器地址：优先使用当前页面来源，本地文件打开时回退到需求文档指定地址
    const DEFAULT_SERVER = 'http://193.112.29.233:8080';
    const BASE_URL = (window.location.protocol !== 'file:')
        ? window.location.origin
        : DEFAULT_SERVER;

    const state = {
        sessions: [],
        models: [],
        currentSessionId: null,
        selectedModel: null,
        isStreaming: false,
        pendingDeleteId: null,
        abortController: null
    };

    const el = {};

    /* ========== 工具函数 ========== */
    function $(id) { return document.getElementById(id); }

    function escapeHtml(str) {
        const div = document.createElement('div');
        div.textContent = str == null ? '' : String(str);
        return div.innerHTML;
    }

    // 时间格式化：今天显示 HH:MM，其余显示完整日期时间
    function formatTimestamp(ts) {
        if (!ts) return '';
        const n = Number(ts);
        const d = new Date(n < 1e12 ? n * 1000 : n);
        const pad = (v) => String(v).padStart(2, '0');
        const now = new Date();
        const sameDay = d.toDateString() === now.toDateString();
        if (sameDay) {
            return `${pad(d.getHours())}:${pad(d.getMinutes())}`;
        }
        return `${d.getFullYear()}-${pad(d.getMonth() + 1)}-${pad(d.getDate())} ${pad(d.getHours())}:${pad(d.getMinutes())}`;
    }

    function truncate(str, len) {
        if (!str) return '';
        const s = str.replace(/\s+/g, ' ').trim();
        return s.length > len ? s.slice(0, len) + '…' : s;
    }

    // 兼容服务器返回的 data 可能是数组，也可能被包了一层 { array: [...] }
    function extractArray(data) {
        if (Array.isArray(data)) return data;
        if (data && Array.isArray(data.array)) return data.array;
        if (data && Array.isArray(data.list)) return data.list;
        return [];
    }

    // 兼容 created_at / create_time 两种字段命名
    function getCreatedAt(s) {
        if (s.created_at != null) return s.created_at;
        if (s.create_time != null) return s.create_time;
        return s.createdAt;
    }
    function getUpdatedAt(s) {
        if (s.updated_at != null) return s.updated_at;
        if (s.update_time != null) return s.update_time;
        return s.updatedAt;
    }

    function toast(msg, type, duration) {
        const t = el.toast;
        t.className = 'toast show' + (type === 'success' || type === 'error' ? ' ' + type : '');
        t.textContent = msg;
        clearTimeout(t._timer);
        t._timer = setTimeout(() => { t.className = 'toast'; }, duration || 2000);
    }

    function copyToClipboard(text) {
        if (navigator.clipboard && window.isSecureContext) {
            return navigator.clipboard.writeText(text);
        }
        return new Promise((resolve, reject) => {
            const ta = document.createElement('textarea');
            ta.value = text;
            ta.style.position = 'fixed';
            ta.style.opacity = '0';
            document.body.appendChild(ta);
            ta.select();
            try {
                document.execCommand('copy');
                resolve();
            } catch (e) {
                reject(e);
            } finally {
                document.body.removeChild(ta);
            }
        });
    }

    /* ========== Markdown 渲染 ========== */
    function initMarked() {
        if (typeof marked === 'undefined') return;

        const renderer = new marked.Renderer();

        renderer.code = function (code, lang) {
            let language = (typeof lang === 'string') ? lang.trim() : '';
            if (!language) language = 'text';
            let highlighted;
            try {
                if (window.hljs) {
                    if (language && hljs.getLanguage(language)) {
                        highlighted = hljs.highlight(code, { language, ignoreIllegals: true }).value;
                    } else {
                        const auto = hljs.highlightAuto(code);
                        highlighted = auto.value;
                        language = auto.language || 'text';
                    }
                } else {
                    highlighted = escapeHtml(code);
                }
            } catch (e) {
                highlighted = escapeHtml(code);
            }
            return (
                '<pre><div class="code-header">' +
                    '<span class="code-lang">' + escapeHtml(language) + '</span>' +
                    '<button class="code-copy-btn" type="button">' +
                        '<svg width="12" height="12" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2" stroke-linecap="round" stroke-linejoin="round"><rect x="9" y="9" width="13" height="13" rx="2"/><path d="M5 15H4a2 2 0 0 1-2-2V4a2 2 0 0 1 2-2h9a2 2 0 0 1 2 2v1"/></svg>' +
                        '<span data-copylabel>复制</span>' +
                    '</button>' +
                '</div><code class="hljs language-' + escapeHtml(language) + '">' +
                    highlighted +
                '</code></pre>'
            );
        };

        marked.setOptions({
            renderer,
            gfm: true,
            breaks: true,
            headerIds: false,
            mangle: false
        });
    }

    function renderMarkdown(text) {
        if (typeof marked !== 'undefined') {
            try {
                return marked.parse(text || '');
            } catch (e) {
                return '<p style="white-space:pre-wrap;word-break:break-word;">' + escapeHtml(text) + '</p>';
            }
        }
        return '<p style="white-space:pre-wrap;word-break:break-word;">' + escapeHtml(text) + '</p>';
    }

    // 给代码块绑定一键复制
    function attachCopyHandlers(container) {
        container.querySelectorAll('pre').forEach((pre) => {
            if (pre.dataset.copyBound) return;
            pre.dataset.copyBound = '1';
            const btn = pre.querySelector('.code-copy-btn');
            const label = btn ? btn.querySelector('[data-copylabel]') : null;
            const codeEl = pre.querySelector('code');
            if (!btn || !codeEl) return;
            btn.addEventListener('click', async () => {
                try {
                    await copyToClipboard(codeEl.innerText);
                    if (label) label.textContent = '已复制';
                    btn.classList.add('copied');
                    setTimeout(() => {
                        if (label) label.textContent = '复制';
                        btn.classList.remove('copied');
                    }, 1500);
                } catch (e) {
                    toast('复制失败', 'error');
                }
            });
        });
    }

    /* ========== API 封装 ========== */
    async function api(method, path, body) {
        const opts = {
            method,
            headers: { 'Accept': 'application/json' }
        };
        if (body !== undefined && body !== null) {
            opts.headers['Content-Type'] = 'application/json';
            opts.body = JSON.stringify(body);
        }
        const res = await fetch(BASE_URL + path, opts);
        if (!res.ok) {
            let msg = 'HTTP ' + res.status;
            try { const e = await res.json(); if (e && e.message) msg = e.message; } catch (_) {}
            throw new Error(msg);
        }
        return res.json();
    }

    function fetchSessions() {
        return api('GET', '/api/sessions');
    }

    function fetchModels() {
        return api('GET', '/api/models');
    }

    function createSession(model) {
        return api('POST', '/api/session', { model });
    }

    function deleteSession(sessionId) {
        return api('DELETE', '/api/session/' + encodeURIComponent(sessionId));
    }

    function fetchHistory(sessionId) {
        return api('GET', '/api/session/' + encodeURIComponent(sessionId) + '/history');
    }

    // 解析 SSE data 字段：服务器可能用 JSON 字符串做了转义（带引号），这里反转义
    function decodeSSEData(raw) {
        let content = raw;
        if (content.startsWith('"')) {
            try {
                content = JSON.parse(content);
            } catch (e) {
                // 非合法 JSON，保留原样
            }
        }
        return content;
    }

    async function streamMessage(sessionId, message, onChunk, onError) {
        const controller = new AbortController();
        state.abortController = controller;
        let hasChunk = false;
        try {
            const res = await fetch(BASE_URL + '/api/message/async', {
                method: 'POST',
                headers: {
                    'Content-Type': 'application/json',
                    'Accept': 'text/event-stream'
                },
                body: JSON.stringify({ session_id: sessionId, message }),
                signal: controller.signal
            });
            if (!res.ok) {
                throw new Error('HTTP ' + res.status);
            }
            const reader = res.body.getReader();
            const decoder = new TextDecoder('utf-8');
            let buffer = '';
            let done = false;
            while (!done) {
                const { done: rd, value } = await reader.read();
                if (rd) break;
                buffer += decoder.decode(value, { stream: true });
                let idx;
                while ((idx = buffer.indexOf('\n\n')) !== -1) {
                    const frame = buffer.slice(0, idx);
                    buffer = buffer.slice(idx + 2);
                    const lines = frame.split('\n');
                    for (const line of lines) {
                        const t = line.trim();
                        if (!t || !t.startsWith('data:')) continue;
                        const data = decodeSSEData(t.slice(5).trimStart());
                        if (data === '[DONE]') {
                            done = true;
                            break;
                        }
                        if (data) {
                            hasChunk = true;
                            onChunk(data);
                        }
                    }
                }
            }
            // 处理缓冲区残留
            if (buffer.trim()) {
                const t = buffer.trim();
                if (t.startsWith('data:')) {
                    const data = decodeSSEData(t.slice(5).trimStart());
                    if (data && data !== '[DONE]') {
                        hasChunk = true;
                        onChunk(data);
                    }
                }
            }
        } catch (err) {
            if (err.name === 'AbortError') return;
            // 已收到部分内容却因 chunked 编码中断，容忍该错误
            if (hasChunk && err instanceof TypeError) return;
            if (onError) onError(err);
        } finally {
            state.abortController = null;
        }
    }

    /* ========== 会话列表渲染 ========== */
    function renderSessions() {
        const list = el.sessionList;
        if (!state.sessions.length) {
            list.innerHTML = '<div class="session-empty">暂无会话，点击"新建对话"开始</div>';
            return;
        }
        const sorted = [...state.sessions].sort((a, b) => Number(getUpdatedAt(b) || 0) - Number(getUpdatedAt(a) || 0));
        list.innerHTML = sorted.map((s) => {
            const first = truncate(s.first_user_message || '（空会话）', 38);
            const time = formatTimestamp(getUpdatedAt(s) || getCreatedAt(s));
            const active = s.id === state.currentSessionId ? 'active' : '';
            return (
                '<div class="session-item ' + active + '" data-sid="' + escapeHtml(s.id) + '">' +
                    '<div class="session-first">' + escapeHtml(first) + '</div>' +
                    '<div class="session-meta">' +
                        '<span class="session-model">' + escapeHtml(s.model || '') + '</span>' +
                        '<div class="session-right">' +
                            '<span class="session-time">' + escapeHtml(time) + '</span>' +
                            '<button class="session-delete" title="更多操作" data-del-sid="' + escapeHtml(s.id) + '">...</button>' +
                        '</div>' +
                    '</div>' +
                '</div>'
            );
        }).join('');

        list.querySelectorAll('.session-item').forEach((item) => {
            item.addEventListener('click', (ev) => {
                if (ev.target.closest('[data-del-sid]')) return;
                switchSession(item.dataset.sid);
            });
        });
        list.querySelectorAll('[data-del-sid]').forEach((btn) => {
            btn.addEventListener('click', (ev) => {
                ev.stopPropagation();
                openContextMenu(btn, btn.dataset.delSid);
            });
        });
    }

    /* ========== 消息渲染 ========== */
    function createMessageNode(role, text, ts) {
        const wrap = document.createElement('div');
        wrap.className = 'message ' + (role === 'user' ? 'user' : 'ai');
        const bubble = document.createElement('div');
        bubble.className = 'msg-bubble';
        const content = document.createElement('div');
        content.className = 'msg-content';
        if (role === 'ai') {
            content.innerHTML = renderMarkdown(text || '');
            attachCopyHandlers(content);
        } else {
            content.style.whiteSpace = 'pre-wrap';
            content.style.wordBreak = 'break-word';
            content.textContent = text || '';
        }
        bubble.appendChild(content);
        wrap.appendChild(bubble);
        if (ts) {
            const tm = document.createElement('div');
            tm.className = 'msg-time';
            tm.textContent = formatTimestamp(ts);
            wrap.appendChild(tm);
        }
        return { wrap, content };
    }

    function setTyping(contentEl) {
        contentEl.innerHTML = '<div class="typing-indicator"><span></span><span></span><span></span></div>';
    }

    function scrollToBottom() {
        const box = el.chatMessages;
        box.scrollTop = box.scrollHeight;
    }

    function renderHistory(messages) {
        el.chatMessages.innerHTML = '';
        const list = Array.isArray(messages) ? messages : [];
        list.forEach((m) => {
            const role = (m.role === 'user') ? 'user' : 'ai';
            const { wrap } = createMessageNode(role, m.content, m.timestamp);
            el.chatMessages.appendChild(wrap);
        });
        scrollToBottom();
    }

    function updateAIRendering(contentEl, fullText) {
        contentEl.innerHTML = renderMarkdown(fullText);
        attachCopyHandlers(contentEl.parentElement || contentEl);
    }

    function getCurrentSessionModel() {
        const s = state.sessions.find((x) => x.id === state.currentSessionId);
        return s ? s.model : '';
    }

    function showWelcome() {
        el.welcomeView.style.display = '';
        el.chatView.style.display = 'none';
    }

    function showChat() {
        el.welcomeView.style.display = 'none';
        el.chatView.style.display = 'flex';
        el.chatModelName.textContent = getCurrentSessionModel() || '';
    }

    async function switchSession(sessionId) {
        state.currentSessionId = sessionId;
        renderSessions();
        showChat();
        el.chatMessages.innerHTML = '';
        el.messageInput.value = '';
        updateInputState();
        try {
            const r = await fetchHistory(sessionId);
            if (r && r.success) {
                renderHistory(r.data || []);
            } else {
                toast((r && r.message) || '获取历史失败', 'error');
            }
        } catch (e) {
            toast('加载历史失败: ' + e.message, 'error');
        }
    }

    /* ========== 模型选择弹窗 ========== */
    function openModelModal() {
        state.selectedModel = null;
        el.modelGrid.innerHTML = '<div class="session-empty">加载中...</div>';
        el.btnConfirmModel.disabled = true;
        el.modelModal.classList.add('show');
        fetchModels().then((r) => {
            if (!r.success) throw new Error(r.message || '加载模型失败');
            state.models = extractArray(r.data);
            renderModelGrid();
        }).catch((e) => {
            el.modelGrid.innerHTML = '<div class="session-empty" style="color:#f87171;">加载失败: ' + escapeHtml(e.message) + '</div>';
        });
    }

    function closeModelModal() {
        el.modelModal.classList.remove('show');
    }

    function renderModelGrid() {
        if (!state.models.length) {
            el.modelGrid.innerHTML = '<div class="session-empty">暂无可选模型</div>';
            return;
        }
        el.modelGrid.innerHTML = state.models.map((m) => {
            const checked = state.selectedModel === m.name ? 'checked' : '';
            const selectedCls = state.selectedModel === m.name ? 'selected' : '';
            return (
                '<label class="model-card ' + selectedCls + '" data-model="' + escapeHtml(m.name) + '">' +
                    '<input type="radio" name="model-radio" value="' + escapeHtml(m.name) + '" ' + checked + '>' +
                    '<div class="model-name">' + escapeHtml(m.name) + '</div>' +
                    '<div class="model-desc">' + escapeHtml(m.desc || '') + '</div>' +
                '</label>'
            );
        }).join('');
        el.modelGrid.querySelectorAll('.model-card').forEach((card) => {
            card.addEventListener('click', () => {
                state.selectedModel = card.dataset.model;
                el.modelGrid.querySelectorAll('.model-card').forEach((c) => {
                    c.classList.toggle('selected', c === card);
                    const input = c.querySelector('input');
                    if (input) input.checked = (c === card);
                });
                el.btnConfirmModel.disabled = false;
            });
        });
    }

    async function confirmCreateSession() {
        if (!state.selectedModel) return;
        try {
            el.btnConfirmModel.disabled = true;
            const r = await createSession(state.selectedModel);
            if (!r.success) throw new Error(r.message || '创建失败');
            const sid = r.data && r.data.session_id;
            const model = (r.data && r.data.model) ? r.data.model : state.selectedModel;
            const nowS = Math.floor(Date.now() / 1000);
            state.sessions.unshift({
                id: sid,
                model,
                created_at: nowS,
                updated_at: nowS,
                message_count: 0,
                first_user_message: ''
            });
            closeModelModal();
            toast('会话已创建', 'success');
            await switchSession(sid);
            el.messageInput.focus();
        } catch (e) {
            toast('创建失败: ' + e.message, 'error');
        } finally {
            el.btnConfirmModel.disabled = !state.selectedModel;
        }
    }

    /* ========== 删除下拉菜单 ========== */
    function openContextMenu(anchorBtn, sessionId) {
        state.pendingDeleteId = sessionId;
        el.contextMenu.classList.add('show');
        const rect = anchorBtn.getBoundingClientRect();
        const menuW = 140;
        const menuH = 42;
        let left = rect.right - menuW;
        let top = rect.bottom + 4;
        if (left < 8) left = 8;
        if (top + menuH > window.innerHeight - 8) top = rect.top - menuH - 4;
        el.contextMenu.style.left = left + 'px';
        el.contextMenu.style.top = top + 'px';
    }

    function closeContextMenu() {
        el.contextMenu.classList.remove('show');
        state.pendingDeleteId = null;
    }

    async function confirmDeleteSession() {
        const sid = state.pendingDeleteId;
        closeContextMenu();
        if (!sid) return;
        try {
            const r = await deleteSession(sid);
            if (!r.success) throw new Error(r.message || '删除失败');
            state.sessions = state.sessions.filter((s) => s.id !== sid);
            if (state.currentSessionId === sid) {
                state.currentSessionId = null;
                showWelcome();
            }
            renderSessions();
            toast('会话已删除', 'success');
        } catch (e) {
            toast('删除失败: ' + e.message, 'error');
        }
    }

    /* ========== 发送消息 ========== */
    function updateInputState() {
        const text = el.messageInput.value;
        el.charCount.textContent = text.length;
        const canSend = text.trim().length > 0 && !state.isStreaming && !!state.currentSessionId;
        el.btnSend.disabled = !canSend;
        const lines = text.split('\n').length;
        el.messageInput.rows = Math.max(1, Math.min(lines, 8));
    }

    async function onSendClick() {
        if (state.isStreaming) return;
        const text = el.messageInput.value;
        if (!text || !text.trim() || !state.currentSessionId) return;
        const sessionId = state.currentSessionId;

        const userTs = Math.floor(Date.now() / 1000);
        const userNodes = createMessageNode('user', text, userTs);
        el.chatMessages.appendChild(userNodes.wrap);

        el.messageInput.value = '';
        updateInputState();
        scrollToBottom();

        const aiNodes = createMessageNode('ai', '');
        el.chatMessages.appendChild(aiNodes.wrap);
        setTyping(aiNodes.content);
        scrollToBottom();

        state.isStreaming = true;
        updateInputState();

        let received = '';
        let errored = false;
        try {
            await streamMessage(sessionId, text,
                (chunk) => {
                    received += chunk;
                    updateAIRendering(aiNodes.content, received);
                    scrollToBottom();
                },
                (err) => {
                    errored = true;
                    if (!received) {
                        updateAIRendering(aiNodes.content, '⚠️ ' + (err.message || '请求出错'));
                    }
                }
            );
            if (!received && !errored) {
                updateAIRendering(aiNodes.content, '（空回复）');
            }
        } catch (e) {
            errored = true;
            if (!received) {
                updateAIRendering(aiNodes.content, '⚠️ 发送失败: ' + (e.message || String(e)));
            }
        } finally {
            if (!aiNodes.wrap.querySelector('.msg-time')) {
                const tm = document.createElement('div');
                tm.className = 'msg-time';
                tm.textContent = formatTimestamp(Math.floor(Date.now() / 1000));
                aiNodes.wrap.appendChild(tm);
            }
            attachCopyHandlers(aiNodes.content.parentElement || aiNodes.content);
            scrollToBottom();
            state.isStreaming = false;
            updateInputState();
            refreshSessionListSilently();
        }
    }

    async function refreshSessionListSilently() {
        try {
            const r = await fetchSessions();
            if (r.success) {
                state.sessions = extractArray(r.data);
                renderSessions();
            }
        } catch (_) {}
    }

    /* ========== 事件绑定 ========== */
    function bindEvents() {
        el.btnNewChatTop.addEventListener('click', openModelModal);
        el.btnNewChatWelcome.addEventListener('click', openModelModal);
        el.btnCancelModel.addEventListener('click', closeModelModal);
        el.btnConfirmModel.addEventListener('click', confirmCreateSession);
        el.modelModal.addEventListener('click', (ev) => {
            if (ev.target === el.modelModal) closeModelModal();
        });

        el.menuDelete.addEventListener('click', confirmDeleteSession);
        document.addEventListener('click', (ev) => {
            if (!el.contextMenu.contains(ev.target)) closeContextMenu();
        });

        el.messageInput.addEventListener('input', updateInputState);
        el.messageInput.addEventListener('keydown', (ev) => {
            if (ev.key === 'Enter' && !ev.shiftKey && !ev.isComposing) {
                ev.preventDefault();
                if (!el.btnSend.disabled) onSendClick();
            }
        });
        el.btnSend.addEventListener('click', onSendClick);

        document.addEventListener('keydown', (ev) => {
            if (ev.key === 'Escape') {
                if (el.modelModal.classList.contains('show')) closeModelModal();
                if (el.contextMenu.classList.contains('show')) closeContextMenu();
            }
        });

        window.addEventListener('resize', closeContextMenu);
    }

    function cacheEls() {
        el.sessionList = $('sessionList');
        el.welcomeView = $('welcomeView');
        el.chatView = $('chatView');
        el.chatModelName = $('chatModelName');
        el.chatMessages = $('chatMessages');
        el.messageInput = $('messageInput');
        el.btnSend = $('btnSend');
        el.charCount = $('charCount');
        el.btnNewChatTop = $('btnNewChatTop');
        el.btnNewChatWelcome = $('btnNewChatWelcome');
        el.modelModal = $('modelModal');
        el.modelGrid = $('modelGrid');
        el.btnCancelModel = $('btnCancelModel');
        el.btnConfirmModel = $('btnConfirmModel');
        el.contextMenu = $('contextMenu');
        el.menuDelete = $('menuDelete');
        el.toast = $('toast');
    }

    async function init() {
        cacheEls();
        initMarked();
        bindEvents();
        updateInputState();
        // 加载 index.html 后自动获取会话列表
        try {
            const r = await fetchSessions();
            if (r && r.success) {
                state.sessions = extractArray(r.data);
            } else {
                toast((r && r.message) || '加载会话失败', 'error');
            }
        } catch (e) {
            toast('无法连接到服务器 (' + BASE_URL + '): ' + e.message, 'error', 3500);
        }
        renderSessions();
    }

    if (document.readyState === 'loading') {
        document.addEventListener('DOMContentLoaded', init);
    } else {
        init();
    }
})();
