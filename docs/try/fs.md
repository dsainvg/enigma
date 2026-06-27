---
hide:
  - navigation
  - toc
  - header
  - footer
---

<style>
  /* Hide scrollbars on the main document */
  body {
    overflow: hidden !important;
  }
  
  /* Reset container margins/paddings */
  .md-container {
    margin: 0 !important;
    padding: 0 !important;
  }
  .md-main__inner {
    max-width: 100% !important;
    margin: 0 !important;
    padding: 0 !important;
  }
  .md-content {
    margin: 0 !important;
    padding: 0 !important;
  }
  .md-content__inner {
    margin: 0 !important;
    padding: 0 !important;
  }
  
  /* Make the iframe take up the full screen */
  .fullscreen-iframe {
    position: fixed;
    top: 0;
    left: 0;
    width: 100vw;
    height: 100vh;
    border: none;
    z-index: 9999;
  }

  /* Floating back button */
  .fs-back-button {
    position: fixed;
    top: 16px;
    right: 16px;
    z-index: 10000;
    display: flex;
    align-items: center;
    gap: 8px;
    padding: 8px 16px;
    background: rgba(15, 23, 42, 0.65);
    backdrop-filter: blur(8px);
    -webkit-backdrop-filter: blur(8px);
    border: 1px solid rgba(255, 255, 255, 0.1);
    border-radius: 30px;
    color: #f8fafc !important;
    font-family: Inter, sans-serif;
    font-size: 14px;
    font-weight: 500;
    text-decoration: none;
    box-shadow: 0 4px 12px rgba(0, 0, 0, 0.25);
    transition: all 0.2s ease-in-out;
  }

  .fs-back-button:hover {
    background: rgba(15, 23, 42, 0.85);
    border-color: rgba(129, 140, 248, 0.5);
    transform: translateY(-1px);
    box-shadow: 0 6px 16px rgba(129, 140, 248, 0.25);
  }
</style>

<a href="../" class="fs-back-button">
  <span class="twemoji"><svg xmlns="http://www.w3.org/2000/svg" viewBox="0 0 24 24"><path d="M20 11v2H8l5.5 5.5-1.42 1.42L4.16 12l7.92-7.92L13.5 5.5 8 11z"/></svg></span> Back to Docs
</a>

<iframe 
  src="https://enigma-ed.streamlit.app/?embed=true" 
  title="Enigma Streamlit Playground Full Screen"
  class="fullscreen-iframe"
  allow="clipboard-write"
  loading="lazy">
</iframe>
