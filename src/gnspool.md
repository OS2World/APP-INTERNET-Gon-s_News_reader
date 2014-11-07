


GNSPOOL(1)         UNIX-OMRON SOFTWARE tool box-Manual      GNSPOOL(1)



名前
	gnspool - gn 用記事取得ユーティリティ

形式
	gnspool [-h hostname] [-d domainname][-epmgyuV]
		-h hostname	ニュースサーバを指定する
		-d domainname	ドメイン名を指定する
		-e		既読記事の消去のみを行なう
		-p		ポストのみをおこなう
		-m		メイルの送信のみをおこなう
		-g		記事の取得のみをおこなう
				-e -p -m -g は組み合わせて使用可
		-f		未読記事数のチェックを手抜きする（危険）
		-y		確認入力なしにポスト／メイルする
		-u		キャンセルされた記事を既読とする
		-C		取寄せた記事はすべて既読とする
		-V		バージョンの表示

特長
	gnspool は .newsrc *1 に従って未読の記事をローカルディスクに取
	り寄せるためのツールである。gn の -s オプションと合わせて使用
	すると、NNTP サーバと接続せずに NetNews を読むことができる。

	*1 .newsrc: 個人毎の既読情報を管理しているファイル
機能説明
	(0) 起動前の準備
		NetNews を読んだことがない（.newsrc を持っていない）ユー
		ザがいきなり gnspool を使用すると、超大量の記事を取り
		寄せることになり、時間的にもディスク容量的にも無理があ
		る。
		gn をオンラインモードで使用し、ニュースサーバ上にある
		ニュースグループを調べ、必要のないニュースグループは
		・u コマンドで unsubscribe（指定したニュースグループは
		  読まない）にしておく
		・gnrc で UNSUBSCRIBE に指定する
		などの処理をおこなう。これにより gnspool 実行時に必要
		な記事だけを取得するようにしておく。

	(1) 初回の起動
		newsrc(-nntpserver) から未読の記事を調べ、購読している
		ニュースグループの未読の記事を、ニュースサーバと同様、
		ニュースグループ毎に記事番号のファイルに置かれる。
		例：	fj.test の 1000 番目の記事は
			$NEWSSPOOL/fj/test/1000

		また active *1 を keyword-newslib/active に転送する。
		もし gnrc でDESCRIPTION ON が指定されていた場合は
		ニュースグループの説明を keyword-newslib/newsgroups に、
		USE_HISTORY ON が指定されていた場合は、
		メッセージＩＤと記事の対応を keyword-newslib/history に、
		置く。
		
		記事を転送するディレクトリは、gnrc の NEWSSPOOL で、
		active などを転送するディレクトリは、gnrc の NEWSLIB で
		変更することができる。gnrc の詳しい説明は gn のマニュ
		アルを参照のこと。

		初回起動時には、active を作成するために
			-f オプションや
			gnrc の FAST_CONNECT 1
		を指定してはいけない。

		*1 active: keyword-newsspool にどんな記事が置いてある
		   かを管理しているファイル

	(2) gn
		取り寄せた記事を読む場合、gn に -s オプションをつけて
		使用する。

		ポストした記事は、$NEWSSPOOL/news.out に、
		リプライメイルは、$NEWSSPOOL/mail.out に
		一時的に置かれる。

	(3) ２回目以降の実行
		初回の起動時と同様の処理を行なう前に
		・記事／メイルの転送
			$NEWSSPOOL から、ポストした記事とリプライメイ
			ルがあるか調べ、もしあれば送信する。
		・既読記事の消去
			$NEWSSPOOL から、既読記事を消す。
		を実行する。

		-f オプションを付加して起動すると、接続時の未読記事数
		の計算が高速になるが、ニュースグループが増減しても対応
		できないため、少なくとも数回に１回は -f オプションを付
		加せずに起動することを推奨する。

		また、-f オプションは購読グループ数が多いとかえって遅
		くなる事があるため、-f オプションの有り無しでどちらが
		速いか、試行錯誤した方がよい。

使用上の留意点
	-u オプションを付加すると、newsrc の更新をおこなう。gnspool が
	動作しているときにニュースリーダを起動すると、排他制御がおこな
	われず既読情報が失われる場合がある。

動作指定
	gnspool は gn と同じ以下の方法で動作を規定することができる
		(1) 初期化ファイル（$HOME/.gnrc-$NNTPSERVER）
		(2) コマンドラインオプション
		(3) 環境変数
		(4) 初期化ファイル（$HOME/.gnrc）
		(5) コンパイル時に指定された値
	詳細については、gn のマニュアルを参照のこと。

関連ファイル
	$HOME/.newsrc
	$HOME/.newsrc-NNTPSERVER
	$HOME/.signature
	$HOME/author_copy
	$HOME/.gnrc
	$HOME/.gnrc-NNTPSERVER


関連コマンド
	gn(1)

BUGS
	DOS の場合、ファイル名／ディレクトリ名は８文字＋３文字に制限さ
	れるため、ニュースグループ名（. から . の間）が８文字を越える
	場合重なる場合がある。

	DOS の場合、ニュースグループ名に con aux などがあると、正常に
	動作しない場合がある。（例：comp.unix.aux）

	$NEWSSPOOL に rmgroup されたニュースグループが残ることがあるた
	め、$NEWSSPOOL を定期的に掃除する必要がある。

Copyright (C) yamasita@omronsoft.co.jp
 	 May.8,1998
著作権は放棄しません。ただし、 営利目的以外の使用／配布に制限は設けません。
