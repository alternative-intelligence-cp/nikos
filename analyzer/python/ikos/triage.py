import json
import os

try:
    import google.genai
except ImportError:
    pass

class TriageClient:
    def __init__(self):
        self.enabled = False
        api_key = os.environ.get('GOOGLE_API_KEY')
        if not api_key:
            return
        
        try:
            self.client = google.genai.Client(api_key=api_key)
            self.enabled = True
            self.model = "gemini-2.5-flash"
        except Exception:
            pass
            
    def _get_source_snippet(self, filepath, line_num, context_lines=5):
        if not filepath or line_num <= 0 or not os.path.isfile(filepath):
            return "<source not available>"
        try:
            with open(filepath, 'r') as f:
                lines = f.readlines()
            start = max(0, line_num - 1 - context_lines)
            end = min(len(lines), line_num + context_lines)
            
            snippet = []
            for i in range(start, end):
                prefix = "->" if i == line_num - 1 else "  "
                snippet.append(f"{i+1:4d} {prefix} {lines[i].rstrip()}")
            return "\n".join(snippet)
        except Exception:
            return "<error reading source>"

    def explain_finding(self, check_kind, result, filepath, line_num, info_dict):
        if not self.enabled:
            return "AI explanation not available (GOOGLE_API_KEY not set)."
            
        snippet = self._get_source_snippet(filepath, line_num)
        
        prompt = f"""
You are an expert C/C++ security researcher and static analysis triage assistant.
Explain the following abstract interpretation finding in plain English to a developer.
Be concise. Do not use mathematical invariant notation like `_1_x_1 ∈ [-128, 127]`.
Focus on the developer's variable names and control flow.

Check Kind: {check_kind}
Result: {result}
Location: {filepath}:{line_num}
Additional Info (JSON from DB): {json.dumps(info_dict)}

Source Code Snippet:
{snippet}

Provide a 2-4 sentence explanation.
"""
        try:
            response = self.client.models.generate_content(model=self.model, contents=prompt)
            return response.text.strip()
        except Exception as e:
            return f"AI explanation failed: {str(e)}"

    def summarize_trace(self, trace_text):
        if not self.enabled:
            return "AI trace summarization not available."
            
        prompt = f"""
You are an expert C/C++ security researcher and static analysis triage assistant.
Summarize the following call stack trace into a single readable sentence or short paragraph.
Highlight the critical decision points, where the issue originates, and where it manifests.

Trace:
{trace_text}
"""
        try:
            response = self.client.models.generate_content(model=self.model, contents=prompt)
            return response.text.strip()
        except Exception as e:
            return f"AI trace summarization failed: {str(e)}"

    def suggest_fix(self, check_kind, filepath, line_num, info_dict):
        if not self.enabled:
            return "AI fix suggestion not available."
            
        snippet = self._get_source_snippet(filepath, line_num, context_lines=10)
        
        prompt = f"""
You are an expert C/C++ security researcher.
Provide a unified diff that fixes the following finding. Only output the unified diff and nothing else.
Do not use markdown code blocks like ```diff, just output the raw diff text.

Check Kind: {check_kind}
Location: {filepath}:{line_num}
Additional Info (JSON from DB): {json.dumps(info_dict)}

Source Code Snippet:
{snippet}
"""
        try:
            response = self.client.models.generate_content(model=self.model, contents=prompt)
            text = response.text.strip()
            if text.startswith("```diff"):
                text = text[7:]
            elif text.startswith("```"):
                text = text[3:]
            if text.endswith("```"):
                text = text[:-3]
            return text.strip()
        except Exception as e:
            return f"AI fix suggestion failed: {str(e)}"

_client = None

def get_client():
    global _client
    if _client is None:
        _client = TriageClient()
    return _client
